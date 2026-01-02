#include <iostream>
#include <fstream>
#include "frontend/Tokens.hpp"
#include "frontend/Parser.hpp"
#include "backend/SymbolTable.hpp"
#include "backend/IR.hpp"

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

    auto tokens = Tokenization::tokenize(buffer);
    auto result = Parser::parseTokens(tokens);

    SymbolTable table(result);
    BuilderIR ir(result);
    std::cout << "_start:\n";
    for(auto& instruction : ir.getCode())
    {
        std::cout << instruction << "\n";
    }
    
    return 0;
}