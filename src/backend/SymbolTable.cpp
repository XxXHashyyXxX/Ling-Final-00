#include "SymbolTable.hpp"

SymbolTable::SymbolTable(const std::vector<std::unique_ptr<AST::Statement>> &statements)
{
    for(const auto& statement : statements)
    {
        validateStatement(statement);
    }
}

unsigned SymbolTable::getOffset() const
{
    return maxOffset;
}

void SymbolTable::enterScope()
{
    Scope scope;
    scope.savedOffset = currentOffset;
    scopes.push_back(std::move(scope));
}

void SymbolTable::leaveScope()
{
    if(scopes.empty()) throw std::runtime_error("[Symbol table] Trying to leave scope that was not entered");
    currentOffset = scopes.back().savedOffset;
    scopes.pop_back();
}

void SymbolTable::declare(AST::VariableData &variable)
{
    if(scopes.empty()) enterScope();

    auto name = variable.getName();
    auto& top = scopes.back();
    if(top.symbols.find(name) != top.symbols.end()) throw std::runtime_error("[Symbol table] Trying to double declare a variable");

    currentOffset += 8;

    variable.resolve(currentOffset);
    top.symbols.emplace(name, currentOffset);
    if(maxOffset < currentOffset) maxOffset = currentOffset;
}

void SymbolTable::resolve(AST::VariableData &variable)
{
    if(scopes.empty()) throw std::runtime_error("[Symbol table] Trying to use variable without scope");

    auto name = variable.getName();
    for(auto it = scopes.rbegin(); it != scopes.rend(); ++it)
    {
        auto& symbols = it->symbols;
        if(symbols.find(name) == symbols.end()) continue;

        auto symbolOffset = symbols.at(name);
        variable.resolve(symbolOffset);
        return;
    }

    throw std::runtime_error("[Symbol table] Trying to use variable without declaration");
}

bool SymbolTable::validateStatement(const std::unique_ptr<AST::Statement> &statement)
{
    if(auto* ptr = dynamic_cast<AST::VariableDeclaration*>(statement.get())) {
        auto& variableData = *dynamic_cast<AST::VariableData*>(ptr);
        auto& expression = ptr->value;
        if(!validateExpression(expression)) return false;
        declare(variableData);
        return true;
    }
    if(auto* ptr = dynamic_cast<AST::VariableAssignment*>(statement.get())) {
        auto& variableData = *dynamic_cast<AST::VariableData*>(ptr);
        resolve(variableData);

        auto& expression = ptr->value;
        return validateExpression(expression);
    }
    if(auto* ptr = dynamic_cast<AST::IfStatement*>(statement.get())) {
        auto& condition = ptr->condition;
        auto& body = ptr->body;
        return validateExpression(condition) && validateStatement(body);
    }
    if(auto* ptr = dynamic_cast<AST::WhileStatement*>(statement.get())) {
        auto& condition = ptr->condition;
        auto& body = ptr->body;
        return validateExpression(condition) && validateStatement(body);
    }
    if(auto* ptr = dynamic_cast<AST::DisplayStatement*>(statement.get())) {
        auto& expression = ptr->expression;
        return validateExpression(expression);
    }
    if(auto* ptr = dynamic_cast<AST::CodeBlock*>(statement.get())) {
        enterScope();
        for(auto& innerStatement : ptr->block)
        {
            if(!validateStatement(innerStatement)) return false;
        }
        leaveScope();
        return true;
    }

    throw std::invalid_argument("Unrecognized statement");
}

bool SymbolTable::validateExpression(const std::unique_ptr<AST::Expression> &expression)
{
    if(dynamic_cast<AST::LiteralValue*>(expression.get()))
        return true;
    if(auto* ptr = dynamic_cast<AST::VariableValue*>(expression.get())) {
        auto& variableData = *dynamic_cast<AST::VariableData*>(ptr);
        resolve(variableData);
        return true;
    }
    if(auto* ptr = dynamic_cast<AST::BinaryOperation*>(expression.get()))
        return validateExpression(ptr->leftOperand) && validateExpression(ptr->rightOperand);
    if(auto* ptr = dynamic_cast<AST::UnaryOperation*>(expression.get()))
        return validateExpression(ptr->operand);
    
    throw std::invalid_argument("Unrecognized expression");
}
