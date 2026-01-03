#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <variant>
#include "../frontend/AST.hpp"
#include <ostream>

class BuilderIR {
public:
    using TempVarID = u_int32_t;
    using LabelID = u_int32_t;

    struct Operand {
        enum class Type {
            Immediate,
            Temporary
        } type;
        
        int immediate = 0;
        TempVarID tempVar = 0;

        static Operand Immediate(int value);
        static Operand TempVar(TempVarID);

        friend std::ostream& operator<<(std::ostream& os, const Operand& op);

    private:
        Operand(Type type);
    };

    struct InstructionLoad {
        InstructionLoad(TempVarID destination, std::string_view sourceSymbol);

        TempVarID destination;
        std::string sourceSymbol;

        friend std::ostream& operator<<(std::ostream& os, const InstructionLoad& i);
    };

    struct InstructionStore {
        InstructionStore(std::string_view destinationSymbol, const Operand& value);

        std::string destinationSymbol;
        Operand value;

        friend std::ostream& operator<<(std::ostream& os, const InstructionStore& i);
    };

    struct InstructionBinaryOperation {
        enum class Operation {
            Addition,
            Subtraction,
            Multiplication,
            Division,
            Modulo
        };

        InstructionBinaryOperation(TempVarID destination, const Operation& operation, const Operand& leftOperand, const Operand& rightOperand);

        TempVarID destination;
        Operation operation;
        Operand leftOperand, rightOperand;

        friend std::ostream& operator<<(std::ostream& os, const InstructionBinaryOperation& i);
    };

    struct InstructionNegation {
        InstructionNegation(TempVarID destination, const Operand& operand);

        TempVarID destination;
        Operand operand;

        friend std::ostream& operator<<(std::ostream& os, const InstructionNegation& i);
    };

    struct InstructionLabel {
        InstructionLabel(LabelID label);

        LabelID label;

        friend std::ostream& operator<<(std::ostream& os, const InstructionLabel& i);
    };

    struct InstructionJump {
        InstructionJump(LabelID destination);

        LabelID destination;

        friend std::ostream& operator<<(std::ostream& os, const InstructionJump& i);
    };

    struct InstructionBranch {
        InstructionBranch(const Operand& condition, LabelID ifTrue, LabelID ifFalse);

        Operand condition;
        LabelID ifTrue;
        LabelID ifFalse;

        friend std::ostream& operator<<(std::ostream& os, const InstructionBranch& i);
    };

    struct InstructionDisplay {
        InstructionDisplay(std::string_view symbol);

        std::string symbol;

        friend std::ostream& operator<<(std::ostream& os, const InstructionDisplay& i);
    };

    using Instruction = std::variant<
        InstructionLoad,
        InstructionStore,
        InstructionBinaryOperation,
        InstructionNegation,
        InstructionLabel,
        InstructionJump,
        InstructionBranch,
        InstructionDisplay
    >;

    Operand lowerExpression(const std::unique_ptr<AST::Expression>& expression);
    void lowerStatement(const std::unique_ptr<AST::Statement>& statement);
    void lowerProgram(const std::vector<std::unique_ptr<AST::Statement>>& statements);

    TempVarID getTempVarsCount() const;

    BuilderIR(const std::vector<std::unique_ptr<AST::Statement>>& program);
    const std::vector<Instruction>& getCode() const;

private:
    std::vector<Instruction> code;

    TempVarID nextTemp = 0;
    LabelID nextLabel = 0;

    TempVarID allocateTempVar();
    LabelID allocateLabel();

    template <class T>
    void emit(T&& instruction);
};

template <class T>
inline void BuilderIR::emit(T &&instruction)
{
    code.emplace_back(std::forward<T>(instruction));   
}

inline std::ostream& operator<<(std::ostream& os, const BuilderIR::Instruction& instruction)
{
    std::visit([&](const auto& v) {
        os << v;
    }, instruction);
    return os;
}