#include "core.h"

#include <cstdio>
#include <cstdlib>
#include <jpeglib.h>

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include <experimental/filesystem>
#include <iostream>
#include <string>
namespace fs = std::experimental::filesystem;

#include "ctpl_stl.h"
#include <turbojpeg.h>

#include <limits>
#include <map>
#include <tuple>

#include "omp.h"

typedef std::chrono::high_resolution_clock Clock;

#if 0

RawImg decompress_memory_jpeg(MemJPEG &mjpg)
{
    RawImg raw;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    jpeg_mem_src(&cinfo, mjpg.jpg_buffer.data(), mjpg.jpg_size);
    int rc = jpeg_read_header(&cinfo, TRUE);

    if (rc != 1)
    {
        std::cerr << "File does not seem to be a normal JPEG" << std::endl;
        exit(EXIT_FAILURE);
    }

    jpeg_start_decompress(&cinfo);

    raw.width = cinfo.output_width;
    raw.height = cinfo.output_height;
    raw.pixel_size = cinfo.output_components;
    raw.bmp_size = raw.width * raw.height * raw.pixel_size;
    raw.bmp_buffer.resize(raw.bmp_size);
    raw.row_stride = raw.width * raw.pixel_size;
    unsigned char *bmp_buffer = raw.bmp_buffer.data();

    while (cinfo.output_scanline < cinfo.output_height)
    {
        unsigned char *buffer_array[1];
        buffer_array[0] = bmp_buffer + (cinfo.output_scanline) * raw.row_stride;

        jpeg_read_scanlines(&cinfo, buffer_array, 1);
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return raw;
}
#endif

RawImg decompress_memory_turbo_jpeg(MemJPEG mjpg)
{
    RawImg raw;
    auto jpegDecompressor = tjInitDecompress();
    constexpr int COLOR_COMPONENTS = 3;

    long unsigned int _jpegSize = mjpg.jpg_size; //!< _jpegSize from above
    unsigned char *_compressedImage =
        mjpg.jpg_buffer.data(); //!< _compressedImage from above

    int jpegSubsamp{-1};
    int width{0};
    int height{0};

    tjDecompressHeader2(jpegDecompressor, _compressedImage, _jpegSize, &width,
                        &height, &jpegSubsamp);

    // unsigned char buffer[width*height*COLOR_COMPONENTS]; //!< will contain
    // the decompressed image

    raw.width = width;
    raw.height = height;
    raw.pixel_size = COLOR_COMPONENTS;
    raw.bmp_size = raw.width * raw.height * raw.pixel_size;
    raw.bmp_buffer.resize(raw.bmp_size);
    raw.row_stride = raw.width * raw.pixel_size;

    tjDecompress2(jpegDecompressor, _compressedImage, _jpegSize,
                  raw.bmp_buffer.data(), width, 0 /*pitch*/, height, TJPF_RGB,
                  TJFLAG_FASTDCT);

    tjDestroy(jpegDecompressor);

    return raw;
}

MemJPEG read_file_jpeg(const std::string &filename)
{
    MemJPEG memjpeg;

    struct stat file_info;

    int rc = stat(filename.c_str(), &file_info);
    if (rc)
    {
        std::cerr << "FAILED to stat source jpg" << std::endl;
        exit(EXIT_FAILURE);
    }
    memjpeg.jpg_size = file_info.st_size;
    memjpeg.jpg_buffer.resize(memjpeg.jpg_size + 100);
    unsigned char *jpg_buffer = memjpeg.jpg_buffer.data();

    int fd = open(filename.c_str(), O_RDONLY);
    int i = 0;
    while (i < memjpeg.jpg_size)
    {
        rc = read(fd, jpg_buffer + i, memjpeg.jpg_size - i);
        //      std::cout << "Input: Read " << rc << "/" << (memjpeg.jpg_size -
        //      i)
        //                << " bytes" << std::endl;
        i += rc;
    }

    close(fd);
    return memjpeg;
}

void write_ppm(const RawImg &raw, const std::string &output)
{
    auto fd = open(output.c_str(), O_CREAT | O_WRONLY, 0666);
    char buf[1024];

    auto rc = sprintf(buf, "P6 %d %d 255\n", static_cast<int>(raw.width),
                      static_cast<int>(raw.height));
    int rs = write(fd, buf, rc); // Write the PPM image header before data
    rs = write(fd, raw.bmp_buffer.data(),
               raw.bmp_size); // Write out all RGB pixel data

    close(fd);
}

std::vector<std::string> get_directory(const std::string &path)
{
    std::vector<std::string> out;
    for (const auto &entry : fs::directory_iterator(path))
        out.push_back(entry.path());
    return out;
}

TimingResult decompress_threadpool(TestData &td)
{
    TimingResult tr;
    ctpl::thread_pool p(td.num_threads);
    std::vector<std::future<void>> results(td.num_tiles);

    std::cout << "Using " << td.num_threads << " threads" << std::endl;

    while (true)
    {
        auto t1 = Clock::now();

        for (int i = 0; i < td.num_tiles; i++)
        {
            results[i] = p.push([i, &td](int id) {
                td.imgs[i] = decompress_memory_turbo_jpeg(td.jpgs[i]);
            });
        }
        for (int i = 0; i < td.num_tiles; i++)
            results[i].wait();
        auto t2 = Clock::now();

        const auto cnt = static_cast<size_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
                .count());

        tr.best_frame_ms = std::min(cnt, tr.best_frame_ms);
        tr.worst_frame_ms = std::max(cnt, tr.worst_frame_ms);
        tr.total_ms += cnt;
        tr.num_frames += 1;

        if (tr.total_ms > td.num_seconds * 1000)
            break;
    }

    return tr;
}

TimingResult decompress_openmp_internal(TestData &td, std::function<void()> f)
{
    TimingResult tr;

    while (true)
    {
        auto t1 = Clock::now();
        f();
        auto t2 = Clock::now();

        const auto cnt = static_cast<size_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
                .count());

        tr.best_frame_ms = std::min(cnt, tr.best_frame_ms);
        tr.worst_frame_ms = std::max(cnt, tr.worst_frame_ms);
        tr.total_ms += cnt;
        tr.num_frames += 1;

        if (tr.total_ms > td.num_seconds * 1000)
            break;
    }
    return tr;
}

#define STRINGIFY(a) #a

#define LOOP_SETTINGS(STATIC_DYNAMIC, CHUNK_SIZE)                                 \
    _Pragma( STRINGIFY( omp parallel for schedule (STATIC_DYNAMIC, CHUNK_SIZE) )) \
    for (int i = 0; i < td.num_tiles; i++)                                        \
    {                                                                             \
        td.imgs[i] = decompress_memory_turbo_jpeg(td.jpgs[i]);                    \
    }

TimingResult decompress_openmp(TestData &td)
{
    size_t chunksize{0};

    if (td.omp_chunk_size >= 1024)
        chunksize = 1024;
    else if (td.omp_chunk_size >= 512)
        chunksize = 512;
    else if (td.omp_chunk_size >= 256)
        chunksize = 256;
    else if (td.omp_chunk_size >= 128)
        chunksize = 128;
    else if (td.omp_chunk_size >= 64)
        chunksize = 64;
    else if (td.omp_chunk_size >= 32)
        chunksize = 32;
    else if (td.omp_chunk_size >= 16)
        chunksize = 16;
    else if (td.omp_chunk_size >= 8)
        chunksize = 8;
    else if (td.omp_chunk_size >= 4)
        chunksize = 4;
    else if (td.omp_chunk_size >= 2)
        chunksize = 2;
    else
        chunksize = 1;

    std::function<void()> f;

    if (td.omp_static)
    {
        std::cout << "Using static scheduling" << std::endl;
        if (chunksize == 1024)
            f = [&]() { LOOP_SETTINGS(static, 1024) };
        else if (chunksize == 512)
            f = [&]() { LOOP_SETTINGS(static, 512) };
        else if (chunksize == 256)
            f = [&]() { LOOP_SETTINGS(static, 256) };
        else if (chunksize == 128)
            f = [&]() { LOOP_SETTINGS(static, 128) };
        else if (chunksize == 64)
            f = [&]() { LOOP_SETTINGS(static, 64) };
        else if (chunksize == 32)
            f = [&]() { LOOP_SETTINGS(static, 32) };
        else if (chunksize == 16)
            f = [&]() { LOOP_SETTINGS(static, 16) };
        else if (chunksize == 8)
            f = [&]() { LOOP_SETTINGS(static, 8) };
        else if (chunksize == 4)
            f = [&]() { LOOP_SETTINGS(static, 4) };
        else if (chunksize == 2)
            f = [&]() { LOOP_SETTINGS(static, 2) };
        else
            f = [&]() { LOOP_SETTINGS(static, 1) };
    }
    else
    {
        std::cout << "Using dynamic scheduling" << std::endl;
        if (chunksize == 1024)
            f = [&]() { LOOP_SETTINGS(dynamic, 1024) };
        else if (chunksize == 512)
            f = [&]() { LOOP_SETTINGS(dynamic, 512) };
        else if (chunksize == 256)
            f = [&]() { LOOP_SETTINGS(dynamic, 256) };
        else if (chunksize == 128)
            f = [&]() { LOOP_SETTINGS(dynamic, 128) };
        else if (chunksize == 64)
            f = [&]() { LOOP_SETTINGS(dynamic, 64) };
        else if (chunksize == 32)
            f = [&]() { LOOP_SETTINGS(dynamic, 32) };
        else if (chunksize == 16)
            f = [&]() { LOOP_SETTINGS(dynamic, 16) };
        else if (chunksize == 8)
            f = [&]() { LOOP_SETTINGS(dynamic, 8) };
        else if (chunksize == 4)
            f = [&]() { LOOP_SETTINGS(dynamic, 4) };
        else if (chunksize == 2)
            f = [&]() { LOOP_SETTINGS(dynamic, 2) };
        else
            f = [&]() { LOOP_SETTINGS(dynamic, 1) };
    }

    std::cout << "Using " << chunksize << " chunk size " << std::endl;

    if (td.num_threads > 0)
    {
        omp_set_num_threads(td.num_threads);
        std::cout << "Using " << td.num_threads << " threads" << std::endl;
    }
    else
    {
        std::cout << "Using default threads" << std::endl;
    }
    std::cout << std::endl;

    return decompress_openmp_internal(td, f);
}

TestData load_test_data(const std::string directory, const size_t width,
                        const size_t height, const size_t num_seconds,
                        const size_t num_threads)
{
    std::cout << "Loading tiles..." << std::endl;

    TestData td;
    const auto paths = get_directory(directory);

    MemJPEG mjpg = read_file_jpeg(paths.front());
    RawImg raw = decompress_memory_turbo_jpeg(mjpg);

    if (false)
    {
        write_ppm(raw, "output.ppm");
    };

    td.width = width;
    td.height = height;
    td.tile_width = raw.width;
    td.tile_height = raw.height;
    td.num_seconds = num_seconds;
    td.num_threads = num_threads;
    td.num_tiles = (td.width / td.tile_width) * (td.height / td.tile_height);
    td.jpgs.resize(td.num_tiles);
    td.imgs.resize(td.num_tiles);

    std::map<std::pair<int, int>, size_t> tile_sizes;

    {
        size_t ctr = 0;
        for (int i = 0; i < td.num_tiles; i++)
        {
            ctr = ctr % paths.size();
            td.jpgs[i] = read_file_jpeg(paths[ctr]);
            const auto raw_i = decompress_memory_turbo_jpeg(td.jpgs[i]);
            tile_sizes[std::make_pair<int, int>(raw_i.width, raw_i.height)] +=
                1;
            ctr++;
        }
    }

    for (const auto &kv : tile_sizes)
        std::cout << kv.first.first << "x" << kv.first.second << ": "
                  << kv.second << std::endl;
    std::cout << std::endl;

    return td;
}

void print_results(const TestData &td, const TimingResult &tr)
{
    std::cout << "Ran " << tr.num_frames << " frames with " << td.num_tiles
              << " tiles in " << tr.total_ms << " ms" << std::endl;

    std::cout << "Best frame: " << tr.best_frame_ms << " ms" << std::endl;
    std::cout << "Worst frame: " << tr.worst_frame_ms << " ms" << std::endl;

    std::cout << "Average fps: " << (tr.num_frames * 1000.0 / tr.total_ms)
              << std::endl;

    const size_t pixels_decoded =
        (td.tile_width * td.tile_height) * td.num_tiles * tr.num_frames;
    std::cout << "Megapixels/s: "
              << (pixels_decoded * 1000.0 * 0.000001 / tr.total_ms)
              << std::endl;
}
