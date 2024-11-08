CC = g++
CFLAGS = -std=c++17
LDFLAGS = g++ -std=c++17 -o image_processing image_processing.cpp `pkg-config --cflags --libs libmagic` `pkg-config --cflags --libs opencv4`
SRC = image_processing.cpp
EXEC = image_processing

all: $(EXEC)

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

verbose: $(SRC)
	$(CC) $(CFLAGS) -v -o $@ $< $(LDFLAGS)

download:
	@if ! command -v brew &> /dev/null; then \
		echo "Homebrew is not installed. Please install it."; \
		/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)" \
		exit 1; \
	fi
	@if ! command -v pkg-config &> /dev/null; then \
		echo "pkg-config is not installed. Please install it."; \
		brew install pkg-config \
		exit 1; \
	fi
	@brew install opencv libmagic

clean:
	rm -f $(EXEC)

