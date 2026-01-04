#pragma once

#include "IR.hpp"
#include "SymbolTable.hpp"

// namespace CodeGen {
//     std::string generateAssembly(std::string_view fileName, const BuilderIR& builderIR, const SymbolTable& symbolTable);
//     std::string generateObjectFile(const std::string& asmFile, std::string_view fileName);
//     void linkObjectFile(const std::string& objectFile, std::string_view fileName);

//     void generateCode(std::string_view fileName, const BuilderIR& builderIR, const SymbolTable& symbolTable);
// };

class CodeGen {
public:
    CodeGen(const BuilderIR& builderIR, const SymbolTable& symbolTable);

    std::string generateAssembly(const std::string& name);
    std::string generateObjectFile(const std::string& name);
    void linkExecutable(const std::string& name);

    void generateExecutable(const std::string& name);

private:
    const BuilderIR& builderIR;
    const SymbolTable& symbolTable;
};