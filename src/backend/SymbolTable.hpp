#pragma once

#include <unordered_set>
#include <string>
#include <string_view>
#include <vector>
#include "../frontend/AST.hpp"
#include <memory>

class SymbolTable {
public:
    SymbolTable(const std::vector<std::unique_ptr<AST::Statement>>& statements);
    ~SymbolTable() = default;

    unsigned getOffset() const;

private:
    struct Scope {
        std::unordered_map<std::string, unsigned> symbols;
        unsigned savedOffset = 0;
    };

    std::vector<Scope> scopes;

    void enterScope();
    void leaveScope();

    void declare(AST::VariableData& variable);
    void resolve(AST::VariableData& variable);

    std::unordered_map<std::string, unsigned> _symbols;
    bool validateStatement(const std::unique_ptr<AST::Statement>& statement);
    bool validateExpression(const std::unique_ptr<AST::Expression>& expression);
    
    unsigned currentOffset = 0;
    unsigned maxOffset = 0;
};