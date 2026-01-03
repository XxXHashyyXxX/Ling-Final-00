#include <iostream>
#include <fstream>
#include "frontend/Tokens.hpp"
#include "frontend/Parser.hpp"
#include "backend/SymbolTable.hpp"
#include "backend/IR.hpp"
#include "backend/CodeGen.hpp"

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cout << "Usage: " << argv[0] << " [fileToCompile]\n";
        return 1;
    }

    std::string srcPath(argv[1]);
    srcPath.append(".ling");
    std::ifstream source(srcPath, std::ios::binary);
    source.seekg(0, std::ios::end);
    std::size_t size = source.tellg();
    source.seekg(0);

    std::string buffer(size, '\0');
    source.read(buffer.data(), size);

    auto tokens = Tokenization::tokenize(buffer);
    auto result = Parser::parseTokens(tokens);

    SymbolTable table(result);
    BuilderIR ir(result);
    CodeGen::generateCode(argv[1], ir, table);
    
    return 0;
}