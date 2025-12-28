#include <iostream>
#include <fstream>
#include "frontend/Tokens.hpp"

using namespace Tokenization;

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cout << "Usage: " << argv[0] << " [fileToCompile]\n";
        return 1;
    }

    std::ifstream source(argv[1], std::ios::binary);
    source.seekg(0, std::ios::end);
    std::size_t size = source.tellg();
    source.seekg(0);

    std::string buffer(size, '\0');
    source.read(buffer.data(), size);

    auto tokens = tokenize(buffer);
    for(const auto& token : tokens) {
        std::cout << token << "\n";
    }
    return 0;
}