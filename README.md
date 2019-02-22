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
./benchmark --directory ../tiles_128 --width 15364 --height 3424 -s 5 -t 8 
Loading tiles...
128x128: 3120

Running benchmark [openmp]...
Using dynamic scheduling
Using 1 chunk size 
Using 8 threads

Ran 85 frames with 3120 tiles in 5025 ms
Best frame: 58 ms
Worst frame: 77 ms
Average fps: 16.9154
Megapixels/s: 864.684
```


Creating tiles:
```
mkdir tiles_64
convert totile.png +gravity -crop 64x64 -background transparent -quality 80 -sampling-factor 1:1:1 -extent 64x64 'tiles_64/tile_%05d.jpg'
```
