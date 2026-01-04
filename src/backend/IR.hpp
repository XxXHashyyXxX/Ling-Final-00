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

    private:
        Operand(Type type);
    };

    struct InstructionLoad {
        InstructionLoad(TempVarID destination, const AST::VariableData& sourceVariable);

        TempVarID destination;
        unsigned offset;
    };

    struct InstructionStore {
        InstructionStore(const AST::VariableData& destinationVariable, const Operand& value);

        unsigned offset;
        Operand value;
    };

    struct InstructionSet {
        InstructionSet(TempVarID destination, int value);

        TempVarID destination;
        int value;
    };

    struct InstructionBinaryOperation {
        enum class Operation {
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

        InstructionBinaryOperation(TempVarID destination, const Operation& operation, const Operand& leftOperand, const Operand& rightOperand);

        TempVarID destination;
        Operation operation;
        Operand leftOperand, rightOperand;
    };

    struct InstructionUnaryOperator {
        enum class Operation {
            Negation,
            Not
        };

        InstructionUnaryOperator(TempVarID destination, const Operand& operand, Operation operation = Operation::Negation);

        TempVarID destination;
        Operand operand;
        Operation operation;
    };

    struct InstructionLabel {
        InstructionLabel(LabelID label);

        LabelID label;
    };

    struct InstructionJump {
        InstructionJump(LabelID destination);

        LabelID destination;
    };

    struct InstructionBranch {
        InstructionBranch(const Operand& condition, LabelID ifTrue, LabelID ifFalse);

        Operand condition;
        LabelID ifTrue;
        LabelID ifFalse;
    };

    struct InstructionCompareEqual {
        InstructionCompareEqual(const Operand& leftOperand, const Operand& rightOperand, LabelID ifEqual, LabelID ifNotEqual);

        Operand leftOperand;
        Operand rightOperand;
        LabelID ifEqual;
        LabelID ifNotEqual;
    };

    struct InstructionCompareLess {
        InstructionCompareLess(const Operand& leftOperand, const Operand& rightOperand, LabelID ifLess, LabelID ifMore);

        Operand leftOperand;
        Operand rightOperand;
        LabelID ifLess;
        LabelID ifMore;
    };

    struct InstructionCompareMore {
        InstructionCompareMore(const Operand& leftOperand, const Operand& rightOperand, LabelID ifMore, LabelID ifLess);

        Operand leftOperand;
        Operand rightOperand;
        LabelID ifMore;
        LabelID ifLess;
    };

    struct InstructionDisplay {
        InstructionDisplay(Operand operand);

        Operand operand;
    };

    struct InstructionBranchCmp {
        enum class ComparisonType {
            Equals,
            NotEquals,
            Greater,
            GreaterEqual,
            Less,
            LessEqual
        };

        InstructionBranchCmp(ComparisonType type, const Operand& leftOperand, const Operand& rightOperand, BuilderIR::LabelID ifTrue, BuilderIR::LabelID ifFalse);

        ComparisonType type;
        Operand leftOperand;
        Operand rightOperand;
        BuilderIR::LabelID ifTrue;
        BuilderIR::LabelID ifFalse;
    };

    using Instruction = std::variant<
        InstructionLoad,
        InstructionStore,
        InstructionBinaryOperation,
        InstructionUnaryOperator,
        InstructionLabel,
        InstructionJump,
        InstructionBranch,
        InstructionDisplay,
        InstructionSet,
        InstructionCompareEqual,
        InstructionCompareMore,
        InstructionCompareLess,
        InstructionBranchCmp
    >;

    Operand lowerExpression(const std::unique_ptr<AST::Expression>& expression);
    void lowerStatement(const std::unique_ptr<AST::Statement>& statement);
    void lowerProgram(const std::vector<std::unique_ptr<AST::Statement>>& statements);

    TempVarID getTempVarsCount() const;

    BuilderIR(const std::vector<std::unique_ptr<AST::Statement>>& program);
    const std::vector<Instruction>& getCode() const;

    void tryOptimize();

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