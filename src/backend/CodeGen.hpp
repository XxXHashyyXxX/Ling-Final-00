#pragma once

#include "IR.hpp"
#include "SymbolTable.hpp"

namespace CodeGen {
    std::string generateAssembly(std::string_view fileName, const BuilderIR& builderIR, const SymbolTable& symbolTable);
    std::string generateObjectFile(const std::string& asmFile, std::string_view fileName);
    void linkObjectFile(const std::string& objectFile, std::string_view fileName);

    void generateCode(std::string_view fileName, const BuilderIR& builderIR, const SymbolTable& symbolTable);
};