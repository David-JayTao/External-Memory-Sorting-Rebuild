#include "external/buffer.h"
#include <iostream>
using namespace external;
int main(int argc, char** argv) {
    try {
        const std::filesystem::path path = argc > 1 ? argv[1] : "week3.bin";
        RunWriter writer(path, 3);
        OutputBuffer output(writer, 2);
        writer.begin_run();
        for (Value value : {1, 4, 4}) output.Put(value);
        output.flush();
        writer.end_run();
        writer.begin_run();
        writer.end_run();
        writer.begin_run();
        output.Put(2);
        output.flush();
        writer.end_run();
        writer.finish();
        RunFile file(path);
        display(file, std::cout, 2);
        std::cout << "Search Run 0 for 4: index " << *Search(file, 0, 4, 2) << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
