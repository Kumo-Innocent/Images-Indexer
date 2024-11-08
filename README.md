# Images Indexer in C++
This little utility creates an HTML that reference every image into directories and sub-directories.<br>
Files are scanned recursively, and it also reference hidden files.

## Arguments
| Name    | Value  | Default | Meaning                                             |
| ------- | ------ | ------- | --------------------------------------------------- |
| width   | int    | 300     | Width of preview image in HTML file                 |
| height  | int    | 250     | Height of preview image in HTML file                |
| threads | int    | 10      | Count of threads at once                            |
| verbose | string | false   | Show logs on runtime (accept : true/yes/1/y)        |
| folder  | string |         | Root folder to scan (example : /Users/name/Pictures |

## Compiling
Here is the command I used to compile this little program _(but you better use Makefile)_:
```
g++ -std=c++17 -o image_processing image_processing.cpp -L/opt/homebrew/Cellar/libmagic/5.45/lib -lmagic -I/opt/homebrew/Cellar/libmagic/5.45/include -I/opt/homebrew/opt/opencv/include/opencv4 -L/opt/homebrew/opt/opencv/lib -lopencv_gapi -lopencv_stitching -lopencv_alphamat -lopencv_aruco -lopencv_bgsegm -lopencv_bioinspired -lopencv_ccalib -lopencv_dnn_objdetect -lopencv_dnn_superres -lopencv_dpm -lopencv_face -lopencv_freetype -lopencv_fuzzy -lopencv_hfs -lopencv_img_hash -lopencv_intensity_transform -lopencv_line_descriptor -lopencv_mcc -lopencv_quality -lopencv_rapid -lopencv_reg -lopencv_rgbd -lopencv_saliency -lopencv_sfm -lopencv_stereo -lopencv_structured_light -lopencv_phase_unwrapping -lopencv_superres -lopencv_optflow -lopencv_surface_matching -lopencv_tracking -lopencv_highgui -lopencv_datasets -lopencv_text -lopencv_plot -lopencv_videostab -lopencv_videoio -lopencv_viz -lopencv_wechat_qrcode -lopencv_xfeatures2d -lopencv_shape -lopencv_ml -lopencv_ximgproc -lopencv_video -lopencv_xobjdetect -lopencv_objdetect -lopencv_calib3d -lopencv_imgcodecs -lopencv_features2d -lopencv_dnn -lopencv_flann -lopencv_xphoto -lopencv_photo -lopencv_imgproc -lopencv_core
```

---
Todo list:
- Save HTML file at chosen path
- Images should be sorted
- Better error handling
- Check for cross platform issues
- HTML file generation at end, to avoid threads error
- Make him portable _(?)_
