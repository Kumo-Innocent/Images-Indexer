#include <iostream>
#include <magic.h>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <functional>
#include <queue>
#include <condition_variable>

using namespace std;
namespace fs = filesystem;
mutex mtx;

class ThreadPool {
    public:
        ThreadPool( size_t length ) : stop( false ), finishedThreads( 0 ), totalThreads( 0 ) {
            for( size_t i = 0; i < length; ++i ) {
                threads.emplace_back( [this] {
                    while( true ) {
                        function<void()> task;
                        {
                            unique_lock<mutex> lock( queueMutex );
                            condition.wait( lock, [this] { return stop || ! tasks.empty(); } );
                            if( stop && tasks.empty() ) return;
                            task = std::move( tasks.front() );
                            tasks.pop();
                        }
                        task();
                        {
                            lock_guard<mutex> lock( counterMutex );
                            ++finishedThreads;
                            if( finishedThreads == totalThreads ) {
                                threadsFinished.notify_one();
                            }
                        }
                    }
                } );
            }
        }
        template<typename F>
        void push( F&& task ) {
            {
                unique_lock<mutex> lock( queueMutex );
                tasks.push( std::forward<F>( task ) );
            }
            condition.notify_one();
        }
        void waitAll( int total ) {
            unique_lock<mutex> lock( counterMutex );
            totalThreads = static_cast<size_t>( total );
            threadsFinished.wait( lock, [this] { return finishedThreads == totalThreads; } );
        }
        ~ThreadPool() {
            {
                unique_lock<mutex> lock( queueMutex );
                stop = true;
            }
            condition.notify_all();
            for( auto& thread : threads )
                thread.join();
        }
    private:
        vector<thread> threads;
        queue<function<void()>> tasks;
        mutex queueMutex;
        mutex counterMutex;
        condition_variable condition;
        condition_variable threadsFinished;
        size_t finishedThreads;
        size_t totalThreads;
        bool stop;
};

bool isFileReadable( const string& filePath ) {
    try {
        fs::file_status status = fs::status( filePath );
        return fs::is_regular_file( status ) && ( ( status.permissions() & fs::perms::owner_read ) != fs::perms::none );
    } catch( const fs::filesystem_error& e ) {
        cerr << "ERREUR: " << e.what() << endl;
        return false;
    }
}

bool isHiddenFile( const string& filePath ) {
    fs::path path( filePath );
    #ifdef _WIN32
    DWORD attributes = GetFileAttributes( path.c_str() );
    if( attributes != INVALID_FILE_ATTRIBUTES ) {
        return attributes & FILE_ATTRIBUTE_HIDDEN;
    }
    return false;
    #else
    return ! path.empty() && path.filename().string().front() == '.';
    #endif
}

bool startsWith( const string& input, const string& needle ) {
    return input.substr( 0, needle.size() ) == needle;
}

void writeHTML( const string& content ) {
    lock_guard<mutex> lock( mtx );
    ofstream outputFile( "index.html", ios::app );
    if( outputFile.is_open() ) {
        outputFile << content;
        outputFile.close();
    }
}

string base64_encode( const unsigned char* to_encode, unsigned int in_len ) {
    string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    string result;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[ 3 ];
    unsigned char char_array_4[ 4 ];
    while( in_len-- ) {
        char_array_3[ i++ ] = *( to_encode++ );
        if( i == 3 ) {
            char_array_4[ 0 ] = ( char_array_3[ 0 ] & 0xFC ) >> 2;
            char_array_4[ 1 ] = (( char_array_3[ 0 ] & 0x03 ) << 4 ) + ( ( char_array_3[ 1 ] & 0xF0 ) >> 4 );
            char_array_4[ 2 ] = (( char_array_3[ 1 ] & 0x0F ) << 2 ) + ( ( char_array_3[ 2 ] & 0xC0 ) >> 6 );
            char_array_4[ 3 ] = char_array_3[ 2 ] & 0x3F;
            for( i = 0; i < 4; i++ ) {
                result += base64_chars[ char_array_4[ i ] ];
            }
            i = 0;
        }
    }
    if( i ) {
        for( j = i; j < 3; j++ ) {
            char_array_3[ j ] = '\0';
        }
        char_array_4[ 0 ] = ( char_array_3[ 0 ] & 0xFC ) >> 2;
        char_array_4[ 1 ] = ( ( char_array_3[ 0 ] & 0x03 ) << 4 ) + ( ( char_array_3[ 1 ] & 0xF0 ) >> 4 );
        char_array_4[ 2 ] = ( ( char_array_3[ 1 ] & 0x0F ) << 2 ) + ( ( char_array_3[ 2 ] & 0xC0 ) >> 6 );
        char_array_4[ 3 ] = char_array_3[ 2 ] & 0x3F;
        for( j = 0; j < i + 1; j++ ) {
            result += base64_chars[ char_array_4[ j ] ];
        }
        while( i++ < 3 ) {
            result += '=';
        }
    }
    return result;
}

cv::Mat resizeImage( const cv::Mat& image, const int& width, const int& height ) {
    cv::Mat resized;
    cv::resize( image, resized, cv::Size( width, height ), 0, 0, cv::INTER_AREA );
    return resized;
}

string encodeToBase64( const cv::Mat& image ) {
    vector<uchar> buffer;
    cv::imencode( ".jpg", image, buffer );
    return base64_encode( buffer.data(), buffer.size() );
}

string getFileMime( const string& filePath ) {
    magic_t magicCookie = magic_open( MAGIC_MIME_TYPE );
    if( magicCookie == NULL ) {
        return "unknown";
    }
    magic_load( magicCookie, NULL );
    const char* mimeType = magic_file( magicCookie, filePath.c_str() );
    string mimeTypeString = mimeType ? mimeType : "unknown";
    magic_close( magicCookie );
    return mimeTypeString;
}

void processImage( const string& filePath, const int& width, const int& height, bool verbose=false ) {
    if( ! isFileReadable( filePath ) ) return;
    fs::directory_entry fileEntry( filePath );
    string mime = getFileMime( filePath );
    string fileName = fileEntry.path().filename().string();
    if( startsWith( mime, "image/" ) ) {
        cv::Mat image = cv::imread( filePath );
        if( ! image.empty() ) {
            cv::Mat resizedImage = resizeImage( image, width, height );
            string base64Data = encodeToBase64( resizedImage );
            string tempContent = R"(
    <div style="position: relative;">
            )";
            tempContent += "<img src=\"data:" + mime + ";base64," + base64Data + "\" alt=\"" + fileName + "\" title=\"" + fileName + "\" />";
            tempContent += R"(
        <div style="position: absolute;bottom: 0;left: 0;display: flex;align-items: center;justify-content: center;width: 100%;height: fit-content;padding-block: 1rem;padding-inline: .75rem;background: rgba(255 255 255 / .5);">
            <span>Nom : )";
            tempContent += fileName;
            tempContent += R"(</span>
        </div>
    </div>
            )";
            writeHTML( tempContent );
            if( verbose ) cout << "Image actuelle : " << filePath << endl;
        }
    }
}

int processImagesInFolder( ThreadPool& threadPool, const string& folderPath, const int& width, const int& height, bool verbose=false, int count=0 ) {
    vector<string> filePaths;
    vector<thread> threads;
    try {
        for( const auto& entry : fs::directory_iterator( folderPath ) ) {
            if( fs::is_regular_file( entry ) && ! isHiddenFile( entry.path().string() ) ) {
                string filePath = entry.path().string();
                ++count;
                threadPool.push( [=]() {
                    processImage( filePath, width, height, verbose );
                } );
            } else if( fs::is_directory( entry ) && ! isHiddenFile( entry.path().string() ) ) {
                count += processImagesInFolder( threadPool, fs::canonical( entry.path() ).string(), width, height, verbose, count );
            }
        }
    } catch( fs::filesystem_error e ) {
        cerr << "HANDLING: " << e.what() << endl;
    }
    return count;
}

int main( int argc, char* argv[] ) {
    unordered_map<string, string> args;
    for( int i = 0; i < argc; ++i ) {
        string argument = argv[ i ];
        if( argument.substr( 0, 2 ) == "--" ) {
            size_t position = argument.find( '=' );
            if( position != string::npos ) {
                string key = argument.substr( 2, position - 2 );
                string value = argument.substr( position + 1 );
                args[ key ] = value;
            }
        }
    }
    int width = 300;
    int height = 250;
    int threadsCount = 10;
    bool verbose = false;
    if( args.count( "width" ) ) {
        width = stoi( args[ "width" ] );
    }
    if( args.count( "height" ) ) {
        height = stoi( args[ "height" ] );
    }
    if(
        args.count( "verbose" ) &&
        (
            args[ "verbose" ] == "true" ||
            args[ "verbose" ] == "yes" ||
            args[ "verbose" ] == "1" ||
            args[ "verbose" ] == "y"
        )
    ) {
        verbose = true;
    }
    if( args.count( "threads") ) {
        threadsCount = stoi( args[ "threads" ] );
    }
    if( args.count( "folder" ) ) {
        cout << "L'indexation peut prendre un certain temps...." << endl;
        ThreadPool threadPool( static_cast<size_t>( threadsCount ) );
        string finalHTML = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Indexation photo</title>
</head>
<body style="background-color: rgb(31 41 55);">
    <div style="display: flex;flex-wrap: wrap;justify-content: space-around;gap: 1.5rem;padding-block: 1rem;padding-inline: 2.5rem;">)";
        ofstream outputFile( "index.html" );
        if( outputFile.is_open() ) {
            outputFile << finalHTML;
            outputFile.close();
        }
        string folderPath = args[ "folder" ];
        int total = processImagesInFolder( threadPool, folderPath, width, height, verbose );
        if( verbose ) cout << total <<" fichiers à traiter." << endl;
        threadPool.waitAll( total );
        finalHTML = R"(
    </div>
</body>
</html>)";
        writeHTML( finalHTML );
        cout << endl << "Fichier index.html généré avec succès.\nIl répertori les images du dossier : " << folderPath << endl;
        return 0;
    } else {
        cout << "Pour utiliser cet indexeur, veuillez préciser au moins l'argument 'folder'" << endl;
        cout << "--> Utilisation: ./image_indexer --folder=/Users/name/Pictures" << endl << endl;
        cout << "Arguments :" << endl << "--folder: Permet de préciser le dossier a indexer" << endl << "--width: Précise la longueur en pixel de l'image retaillée" << endl << "--height: Précise la hauteur en pixel de l'image retaillée" << endl << "--verbose: Affiche l'image actuellement traitée (préciser 'y'/'yes'/'1')." << endl << "--threads: Nombre de threads en simulatané (défaut: 10)" << endl;
        return 1;
    }
}
