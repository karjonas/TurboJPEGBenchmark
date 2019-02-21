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
    const auto paths = get_directory(directory);

    MemJPEG mjpg = read_file_jpeg(paths.front());
    RawImg raw = decompress_memory_turbo_jpeg(mjpg);

    if (true)
    {
        write_ppm(raw, "output.ppm");
    };

    TestData td;

    td.width = static_cast<size_t>(std::stoi(argv[2]));
    td.height = static_cast<size_t>(std::stoi(argv[3]));
    td.tile_width = raw.width;
    td.tile_height = raw.height;
    td.num_frames = static_cast<size_t>(std::stoi(argv[4]));
    td.num_threads = static_cast<size_t>(std::stoi(argv[5]));
    td.num_tiles = (td.width / td.tile_width) * (td.height / td.tile_height);
    td.jpgs.resize(td.num_tiles);
    td.imgs.resize(td.num_tiles);

    {
        std::cout << "Loading tiles" << std::endl;
        size_t ctr = 0;
        for (int i = 0; i < td.num_tiles; i++)
        {
            ctr = ctr % paths.size();
            td.jpgs[i] = read_file_jpeg(paths[ctr]);
            ctr++;
        }
    }

    std::cout << "Running benchmark" << std::endl;
    const auto tr = decompress_threadpool(td);
    print_results(td, tr);

    return EXIT_SUCCESS;
}
