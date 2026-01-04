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

    void addSymbol(const std::string& symbol);
    bool doesSymbolExist(const std::string& symbol) const;
    unsigned getOffset() const;
    unsigned getOffset(const std::string& symbol) const;

    std::unordered_set<std::string>::const_iterator begin() const;
    std::unordered_set<std::string>::const_iterator end() const;

private:
    std::unordered_set<std::string> symbols;
    std::unordered_map<std::string, unsigned> _symbols;
    bool validateStatement(const std::unique_ptr<AST::Statement>& statement);
    bool isExpressionValid(const std::unique_ptr<AST::Expression>& expression) const;
    unsigned currentOffset = 0;
};