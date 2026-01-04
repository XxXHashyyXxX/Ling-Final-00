#include "SymbolTable.hpp"

SymbolTable::SymbolTable(const std::vector<std::unique_ptr<AST::Statement>> &statements)
{
    for(const auto& statement : statements)
    {
        validateStatement(statement);
    }
}

void SymbolTable::addSymbol(const std::string &symbol)
{
    auto status = _symbols.insert({symbol, currentOffset}).second;
    if(!status) throw std::runtime_error("A symbol defined twice");

    currentOffset += 8;
}

bool SymbolTable::doesSymbolExist(const std::string &symbol) const
{
    return _symbols.find(symbol) != _symbols.end();
}

unsigned SymbolTable::getOffset() const
{
    return currentOffset;
}

unsigned SymbolTable::getOffset(const std::string &symbol) const
{
    if(!doesSymbolExist(symbol)) throw std::runtime_error("[Symbol table] Unrecognized symbol");

    return _symbols.at(symbol);
}

std::unordered_set<std::string>::const_iterator SymbolTable::begin() const
{
    return symbols.begin();
}

std::unordered_set<std::string>::const_iterator SymbolTable::end() const
{
    return symbols.end();
}

bool SymbolTable::validateStatement(const std::unique_ptr<AST::Statement> &statement)
{
    if(auto* ptr = dynamic_cast<AST::VariableDeclaration*>(statement.get())) {
        auto symbol = ptr->identificator;
        auto& expression = ptr->value;
        if(doesSymbolExist(symbol) || !isExpressionValid(expression)) return false;
        addSymbol(symbol);
        return true;
    }
    if(auto* ptr = dynamic_cast<AST::VariableAssignment*>(statement.get())) {
        auto symbol = ptr->identificator;
        auto& expression = ptr->value;
        return doesSymbolExist(symbol) && isExpressionValid(expression);
    }
    if(auto* ptr = dynamic_cast<AST::IfStatement*>(statement.get())) {
        auto& condition = ptr->condition;
        auto& body = ptr->body;
        return isExpressionValid(condition) && validateStatement(body);
    }
    if(auto* ptr = dynamic_cast<AST::WhileStatement*>(statement.get())) {
        auto& condition = ptr->condition;
        auto& body = ptr->body;
        return isExpressionValid(condition) && validateStatement(body);
    }
    if(auto* ptr = dynamic_cast<AST::DisplayStatement*>(statement.get())) {
        auto symbol = ptr->identificator;
        return doesSymbolExist(symbol);
    }
    if(auto* ptr = dynamic_cast<AST::CodeBlock*>(statement.get())) {
        for(auto& innerStatement : ptr->block)
        {
            if(!validateStatement(innerStatement)) return false;
        }
        return true;
    }

    throw std::invalid_argument("Unrecognized statement");
}

bool SymbolTable::isExpressionValid(const std::unique_ptr<AST::Expression> &expression) const
{
    if(dynamic_cast<AST::LiteralValue*>(expression.get()))
        return true;
    if(auto* ptr = dynamic_cast<AST::VariableValue*>(expression.get()))
        return doesSymbolExist(ptr->identificator);
    if(auto* ptr = dynamic_cast<AST::BinaryOperation*>(expression.get()))
        return isExpressionValid(ptr->leftOperand) && isExpressionValid(ptr->rightOperand);
    if(auto* ptr = dynamic_cast<AST::UnaryOperation*>(expression.get()))
        return isExpressionValid(ptr->operand);
    
    throw std::invalid_argument("Unrecognized expression");
}
