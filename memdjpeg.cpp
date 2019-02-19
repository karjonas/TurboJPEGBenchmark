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

typedef std::chrono::high_resolution_clock Clock;

struct MemJPEG
{
    size_t jpg_size{0};
    std::vector<unsigned char> jpg_buffer;
};

struct RawImg
{
    size_t width{0};
    size_t height{0};
    size_t pixel_size{0};
    size_t bmp_size{0};
    size_t row_stride{0};
    std::vector<unsigned char> bmp_buffer;
};

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
        std::cout << "Input: Read " << rc << "/" << (memjpeg.jpg_size - i)
                  << " bytes" << std::endl;
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

struct TimingResult
{
    size_t best_frame_ms{std::numeric_limits<size_t>::max()};
    size_t worst_frame_ms{0};
    size_t total_ms{0};
};

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "USAGE: %s filename.jpg num_frames num_tiles\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    const size_t num_frames = static_cast<size_t>(std::stoi(argv[2]));
    const size_t num_tiles = static_cast<size_t>(std::stoi(argv[3]));

    MemJPEG mjpg = read_file_jpeg(argv[1]);
    if (false)
    {
        RawImg raw = decompress_memory_jpeg(mjpg);
        write_ppm(raw, "output.ppm");
    };

    std::vector<RawImg> imgs(num_tiles);

    TimingResult tr;
    for (int frame = 0; frame < num_frames; frame++)
    {
        auto t1 = Clock::now();
#pragma omp parallel for
        for (int i = 0; i < num_tiles; i++)
        {
            imgs[i] = decompress_memory_jpeg(mjpg);
        }
        auto t2 = Clock::now();

        const auto cnt = static_cast<size_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1)
                .count());

        tr.best_frame_ms = std::min(cnt, tr.best_frame_ms);
        tr.worst_frame_ms = std::max(cnt, tr.worst_frame_ms);
        tr.total_ms += cnt;
    }

    std::cout << "Ran " << num_frames << " frames with " << num_tiles
              << " tiles in " << tr.total_ms << " ms" << std::endl;

    std::cout << "Best frame: " << tr.best_frame_ms << " ms" << std::endl;
    std::cout << "Worst frame: " << tr.worst_frame_ms << " ms" << std::endl;

    return EXIT_SUCCESS;
}
