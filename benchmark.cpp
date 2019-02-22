#include "core.h"
#include <CLI/CLI.hpp>

#include <iostream>

int main(int argc, char *argv[])
{
    CLI::App app{"TurboJPEG benchmark"};

    std::string directory = "";
    size_t width{0};
    size_t height{0};
    size_t num_seconds{0};
    size_t num_threads{0};
    bool use_threadpool{false};

    app.add_option("-d,--directory", directory,
                   "Directory containing jpeg tiles")
        ->required();
    app.add_option("--width", width, "Frame width")->required();
    app.add_option("--height", height, "Frame height")->required();
    app.add_option("-s,--seconds", num_seconds, "Num seconds to run test")
        ->required();
    app.add_option("-t,--threads", num_threads, "Num threads");
    app.add_flag("--threadpool", use_threadpool, "Use threadpool");

    CLI11_PARSE(app, argc, argv);
    if (use_threadpool && num_threads == 0)
    {
        std::cerr << "Using threadpool with zero threads" << std::endl;
        return EXIT_FAILURE;
    }

    auto td =
        load_test_data(directory, width, height, num_seconds, num_threads);

    TimingResult tr;

    if (use_threadpool)
    {
        std::cout << "Running benchmark [threadpool]..." << std::endl;
        tr = decompress_threadpool(td);
    }
    else
    {
        std::cout << "Running benchmark [openmp]..." << std::endl;
        tr = decompress_openmp(td);
    }
    print_results(td, tr);

    return EXIT_SUCCESS;
}
