#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

#include "types.h"
#include "parser.h"
#include "parquet_writer.h"

int main(int argc, char* argv[])
{
    if (argc < 3) {
        std::cout << "Usage: itch_to_parquet [input-file] [output-dir]\n";
        return 0;
    }

    const std::string input_path{argv[1]};
    const std::string output_dir{argv[2]};

    try {
        std::filesystem::create_directories(output_dir);

        ITCHReader reader{input_path};
        ParquetExportManager export_manager{output_dir};
        uint64_t counter{};
        auto start = std::chrono::high_resolution_clock::now();

        auto handler = [&export_manager](Message&& msg) {
            export_manager.write(msg);
        };

        reader.read_messages(handler, counter);
        export_manager.finish();

        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();

        std::cout << "Messages: " << counter << '\n';
        std::cout << "Time: " << duration << "ms\n";
        std::cout << "Throughput: " << ((duration > 0) ? (counter * 1000) / duration : 0) << " msg/s\n";
    } catch (const std::exception& e) {
        std::cerr << "itch_to_parquet: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
