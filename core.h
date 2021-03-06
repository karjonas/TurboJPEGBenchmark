#include <limits>
#include <string>
#include <vector>

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

struct TimingResult
{
    size_t best_frame_ms{std::numeric_limits<size_t>::max()};
    size_t worst_frame_ms{0};
    size_t total_ms{0};
    size_t num_frames{0};
};

struct TestData
{
    size_t width{0};
    size_t height{0};
    size_t tile_width{0};
    size_t tile_height{0};
    size_t num_seconds{0};
    size_t num_threads{0};
    size_t num_tiles{0};

    bool omp_static{false};
    bool omp_dynamic{false};
    size_t omp_chunk_size{0};

    std::vector<MemJPEG> jpgs;
    std::vector<RawImg> imgs;
};

std::vector<std::string> get_directory(const std::string &path);
RawImg decompress_memory_turbo_jpeg(MemJPEG mjpg);
MemJPEG read_file_jpeg(const std::string &filename);
void write_ppm(const RawImg &raw, const std::string &output);
TimingResult decompress_threadpool(TestData &td);
TimingResult decompress_openmp(TestData &td);
TestData load_test_data(const std::string directory, const size_t width,
                        const size_t height, const size_t num_seconds,
                        const size_t num_threads);
void print_results(const TestData &td, const TimingResult &tr);
