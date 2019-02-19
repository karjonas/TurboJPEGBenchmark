# TurboJPEGBenchmark

This application will benchmark decoding jpeg images in parallel using libjpeg-turbo. It takes an input image and decodes it multiple times in parallel and repeates this process a number of times.

Compiling:

```
mkdir build
cd build
cmake ..
make
```

Running:

```
./decompress ../test.jpeg 100 650

Input: Read 12649/12649 bytes
Ran 100 frames with 650 tiles in 3820 ms
Best frame: 36 ms
Worst frame: 83 ms
Average fps: 26.178
```