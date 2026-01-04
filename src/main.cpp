#include <iostream>
#include <fstream>
#include "frontend/Tokens.hpp"
#include "frontend/Parser.hpp"
#include "backend/SymbolTable.hpp"
#include "backend/IR.hpp"
#include "backend/CodeGen.hpp"

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cout << "Usage: " << argv[0] << " [fileToCompile]\n";
        return 1;
    }

    std::string src;
    bool fullCompile = true;

    for(int i = 1; i < argc; ++i)
    {
        if(argv[i][0] != '-')
        {
            src = argv[i];
            continue;
        }

        if(argv[i] == std::string("-s"))
            fullCompile = false;
    }

    std::string srcPath = src + ".ling";
    std::ifstream source(srcPath, std::ios::binary);
    source.seekg(0, std::ios::end);
    std::size_t size = source.tellg();
    source.seekg(0);

    std::string buffer(size, '\0');
    source.read(buffer.data(), size);

    auto tokens = Tokenization::tokenize(buffer);
    std::vector<Token>::const_iterator it = tokens.begin();
    auto result = Parser::parseTokens(tokens, it);

    SymbolTable table(result);
    BuilderIR ir(result);

    CodeGen gen(ir, table);

    if(fullCompile) gen.generateExecutable(src);
    else gen.generateAssembly(src);
    
    return 0;
}