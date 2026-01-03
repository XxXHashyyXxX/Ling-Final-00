#pragma once
#include <string>
#include <memory>
#include <optional>
#include <unordered_map>
#include <functional>

namespace AST {

    // ===== Base classes =====
    struct Statement { virtual ~Statement() = 0; };
    struct Expression {
        virtual ~Expression() = 0;
        virtual std::optional<int> getValue() const = 0;
    };

    // ===== Statements =====
    struct VariableDeclaration : public Statement {
        VariableDeclaration(std::string_view identificator, std::unique_ptr<Expression> value);
        ~VariableDeclaration() = default;

        std::string identificator;
        std::unique_ptr<Expression> value;
    };
    struct VariableAssignment : public Statement {
        VariableAssignment(std::string_view identificator, std::unique_ptr<Expression> value);
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
        DisplayStatement(std::string_view identificator);
        ~DisplayStatement() = default;

        std::string identificator;
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
    struct VariableValue : public Expression {
        VariableValue(std::string_view identificator);
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
            Modulo
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
            {OperationType::Modulo,         [](int a, int b) { return a % b; }}
        }};
    };
    struct UnaryOperation : public Expression {
        enum class OperationType {
            Identity,
            Negation
        };

        UnaryOperation(OperationType operation, std::unique_ptr<Expression> operand);
        ~UnaryOperation() = default;

        std::optional<int> getValue() const override;

        OperationType operation;
        std::unique_ptr<Expression> operand;

        inline static const std::unordered_map<OperationType, std::function<int(int)>> operations = {{
            {OperationType::Identity, [](int a) { return a; }},
            {OperationType::Negation, [](int a) { return -a; }}
        }};
    };
};