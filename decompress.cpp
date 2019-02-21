#include "core.h"

#include <iostream>

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        fprintf(stderr,
                "USAGE: %s directory width height num_frames num_threads\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    const std::string directory = argv[1];
    const size_t width = static_cast<size_t>(std::stoi(argv[2]));
    const size_t height = static_cast<size_t>(std::stoi(argv[3]));
    const size_t num_frames = static_cast<size_t>(std::stoi(argv[4]));
    const size_t num_threads = static_cast<size_t>(std::stoi(argv[5]));

    auto td = load_test_data(directory, width, height, num_frames, num_threads);
    std::cout << "Running benchmark" << std::endl;
    const auto tr = decompress_threadpool(td);
    print_results(td, tr);

    return EXIT_SUCCESS;
}
