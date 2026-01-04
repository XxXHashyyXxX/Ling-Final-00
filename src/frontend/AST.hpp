#pragma once
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>
#include <functional>
#include "Tokens.hpp"

namespace AST {

    // ===== Base classes =====
    struct Declaration { virtual ~Declaration() = 0; };
    struct Statement { virtual ~Statement() = 0; };
    struct Expression {
        virtual ~Expression() = 0;
        virtual std::optional<int> getValue() const = 0;
    };
    struct VariableData {
        virtual ~VariableData() = 0;
        explicit VariableData(const Tokenization::Token& token); 

        unsigned offset = 0;
        bool resolved = false;
        
        std::string getName() const;
        void resolve(unsigned offset);
    
        const Tokenization::Token& token;
    };

    // ===== Statements =====
    struct LocalVariableDeclaration : public Statement, public VariableData {
        LocalVariableDeclaration(const Tokenization::Token& identificator, std::unique_ptr<Expression> value);
        ~LocalVariableDeclaration() = default;

        std::string identificator;
        std::unique_ptr<Expression> value;
    };
    struct VariableAssignment : public Statement, public VariableData {
        VariableAssignment(const Tokenization::Token& identificator, std::unique_ptr<Expression> value);
        ~VariableAssignment() = default;

        std::string identificator;
        std::unique_ptr<Expression> value;
    };
    struct IfStatement : public Statement {
        IfStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body);
        ~IfStatement() = default;

        std::unique_ptr<Expression> condition;
        std::unique_ptr<Statement> body;
    };
    struct WhileStatement : public Statement {
        WhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body);
        ~WhileStatement() = default;

        std::unique_ptr<Expression> condition;
        std::unique_ptr<Statement> body;
    };
    struct DisplayStatement : public Statement {
        DisplayStatement(std::unique_ptr<Expression> expression);
        ~DisplayStatement() = default;

        std::unique_ptr<Expression> expression;
    };
    struct CodeBlock : public Statement {
        CodeBlock(std::vector<std::unique_ptr<Statement>> block);
        ~CodeBlock() = default;

        std::vector<std::unique_ptr<Statement>> block;
    };

    // ===== Expressions =====
    struct LiteralValue : public Expression {
        LiteralValue(std::string_view value);
        ~LiteralValue() = default;

        std::optional<int> getValue() const override;

        int value;
    };
    struct VariableValue : public Expression, public VariableData {
        VariableValue(const Tokenization::Token& identificator);
        ~VariableValue() = default;

        std::optional<int> getValue() const override;

        std::string identificator;
    };
    struct BinaryOperation : public Expression {
        enum class OperationType {
            Addition,
            Subtraction,
            Multiplication,
            Division,
            Modulo,
            And,
            Or,
            Equals,
            NotEquals,
            GreaterThan,
            GreaterEqual,
            LessThan,
            LessEqual
        };

        BinaryOperation(OperationType operation, std::unique_ptr<Expression> leftOperand, std::unique_ptr<Expression> rightOperand);
        ~BinaryOperation() = default;

        std::optional<int> getValue() const override;

        OperationType operation;
        std::unique_ptr<Expression> leftOperand;
        std::unique_ptr<Expression> rightOperand;
    
        inline static const std::unordered_map<OperationType, std::function<int(int, int)>> operations = {{
            {OperationType::Addition,       [](int a, int b) { return a + b; }},
            {OperationType::Division,       [](int a, int b) { return a / b; }},
            {OperationType::Multiplication, [](int a, int b) { return a * b; }},
            {OperationType::Subtraction,    [](int a, int b) { return a - b; }},
            {OperationType::Modulo,         [](int a, int b) { return a % b; }},
            {OperationType::And,            [](int a, int b) { return a && b; }},
            {OperationType::Or,             [](int a, int b) { return a || b; }},
            {OperationType::Equals,         [](int a, int b) { return a == b; }},
            {OperationType::NotEquals,      [](int a, int b) { return a != b; }},
            {OperationType::GreaterEqual,   [](int a, int b) { return a >= b; }},
            {OperationType::GreaterThan,    [](int a, int b) { return a > b; }},
            {OperationType::LessEqual,      [](int a, int b) { return a <= b; }},
            {OperationType::LessThan,       [](int a, int b) { return a < b; }}
        }};
    };
    struct UnaryOperation : public Expression {
        enum class OperationType {
            Identity,
            Negation,
            Not
        };

        UnaryOperation(OperationType operation, std::unique_ptr<Expression> operand);
        ~UnaryOperation() = default;

        std::optional<int> getValue() const override;

        OperationType operation;
        std::unique_ptr<Expression> operand;

        inline static const std::unordered_map<OperationType, std::function<int(int)>> operations = {{
            {OperationType::Identity,   [](int a) { return a; }},
            {OperationType::Negation,   [](int a) { return -a; }},
            {OperationType::Not,        [](int a) { return !a; }}
        }};
    };
};