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


Creating tiles:
```
mkdir tiles_64
convert totile.png +gravity -crop 64x64 -background transparent -quality 80 -sampling-factor 1:1:1 -extent 64x64 'tiles_64/tile_%05d.jpg'
```
