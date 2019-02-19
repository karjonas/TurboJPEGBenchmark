# TurboJPEGBenchmark

This application will benchmark decoding jpeg images in parallel using libjpeg-turbo. It takes an input image and decodes it multiple times in parallel and repeates this process a number of times.

To compile and run:

```
 mkdir build
 cd build
 cmake ..
 make
 ./decompress ../test.jpeg 100 50
```
