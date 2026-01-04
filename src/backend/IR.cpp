#include "IR.hpp"
#include <unordered_set>

BuilderIR::Operand BuilderIR::Operand::Immediate(int value)
{
    Operand operand{Type::Immediate};
    operand.immediate = value;
    return operand;
}

BuilderIR::Operand BuilderIR::Operand::TempVar(TempVarID tempVar)
{
    Operand operand{Type::Temporary};
    operand.tempVar = tempVar;
    return operand;
}

BuilderIR::Operand::Operand(BuilderIR::Operand::Type type)
    : type(type) {}

inline static std::unordered_map<AST::BinaryOperation::OperationType, BuilderIR::InstructionBinaryOperation::Operation> astBinopToIrBinop = {{
    {AST::BinaryOperation::OperationType::Addition, BuilderIR::InstructionBinaryOperation::Operation::Addition},
    {AST::BinaryOperation::OperationType::Subtraction, BuilderIR::InstructionBinaryOperation::Operation::Subtraction},
    {AST::BinaryOperation::OperationType::Multiplication, BuilderIR::InstructionBinaryOperation::Operation::Multiplication},
    {AST::BinaryOperation::OperationType::Division, BuilderIR::InstructionBinaryOperation::Operation::Division},
    {AST::BinaryOperation::OperationType::Modulo, BuilderIR::InstructionBinaryOperation::Operation::Modulo},
    {AST::BinaryOperation::OperationType::And, BuilderIR::InstructionBinaryOperation::Operation::And},
    {AST::BinaryOperation::OperationType::Or, BuilderIR::InstructionBinaryOperation::Operation::Or},
    {AST::BinaryOperation::OperationType::Equals, BuilderIR::InstructionBinaryOperation::Operation::Equals},
    {AST::BinaryOperation::OperationType::NotEquals, BuilderIR::InstructionBinaryOperation::Operation::NotEquals},
    {AST::BinaryOperation::OperationType::GreaterEqual, BuilderIR::InstructionBinaryOperation::Operation::GreaterEqual},
    {AST::BinaryOperation::OperationType::GreaterThan, BuilderIR::InstructionBinaryOperation::Operation::GreaterThan},
    {AST::BinaryOperation::OperationType::LessEqual, BuilderIR::InstructionBinaryOperation::Operation::LessEqual},
    {AST::BinaryOperation::OperationType::LessThan, BuilderIR::InstructionBinaryOperation::Operation::LessThan}
}};

inline static std::unordered_map<AST::UnaryOperation::OperationType, BuilderIR::InstructionUnaryOperator::Operation> astUnopToIrUnop = {{
    {AST::UnaryOperation::OperationType::Negation, BuilderIR::InstructionUnaryOperator::Operation::Negation},
    {AST::UnaryOperation::OperationType::Not, BuilderIR::InstructionUnaryOperator::Operation::Not}
}};

BuilderIR::Operand BuilderIR::lowerExpression(const std::unique_ptr<AST::Expression>& expression)
{
    if(auto value = expression->getValue())
    {
        return Operand::Immediate(value.value());
    }

    if(auto* identificator = dynamic_cast<AST::VariableValue*>(expression.get()))
    {
        TempVarID temp = allocateTempVar();
        auto& variable = *dynamic_cast<AST::VariableData*>(identificator);
        emit(InstructionLoad(temp, variable));
        return Operand::TempVar(temp);
    }

    if(auto* unary = dynamic_cast<AST::UnaryOperation*>(expression.get()))
    {
        auto operation = unary->operation;
        Operand operand = lowerExpression(unary->operand);

        if(operation == AST::UnaryOperation::OperationType::Identity) return operand;
        auto operationIR = astUnopToIrUnop.at(operation);

        switch(operationIR)
        {
            case BuilderIR::InstructionUnaryOperator::Operation::Negation: {
                TempVarID temp = allocateTempVar();
                emit(InstructionUnaryOperator(temp, operand));
                return Operand::TempVar(temp);
            }
            case BuilderIR::InstructionUnaryOperator::Operation::Not: {
                TempVarID temp = allocateTempVar();
                LabelID valueTrue = allocateLabel();
                LabelID valueFalse = allocateLabel();
                LabelID done = allocateLabel();
                emit(InstructionBranch(operand, valueFalse, valueTrue));
                emit(InstructionLabel(valueFalse));
                emit(InstructionSet(temp, 0));
                emit(InstructionJump(done));
                emit(InstructionLabel(valueTrue));
                emit(InstructionSet(temp, 1));
                emit(InstructionLabel(done));
                return Operand::TempVar(temp);
            }
        }
    }

    if(auto* binary = dynamic_cast<AST::BinaryOperation*>(expression.get()))
    {
        Operand leftOperand = lowerExpression(binary->leftOperand);
        Operand rightOperand = lowerExpression(binary->rightOperand);

        auto operation = astBinopToIrBinop.at(binary->operation);
        switch(operation)
        {
            case BuilderIR::InstructionBinaryOperation::Operation::And: {
                TempVarID temp = allocateTempVar();
                LabelID keepChecking = allocateLabel();
                LabelID valueTrue = allocateLabel();
                LabelID valueFalse = allocateLabel();
                LabelID done = allocateLabel();
                emit(InstructionBranch(leftOperand, keepChecking, valueFalse));
                emit(InstructionLabel(keepChecking));
                emit(InstructionBranch(rightOperand, valueTrue, valueFalse));
                emit(InstructionLabel(valueTrue));
                emit(InstructionSet(temp, 1));
                emit(InstructionJump(done));
                emit(InstructionLabel(valueFalse));
                emit(InstructionSet(temp, 0));
                emit(InstructionLabel(done));
                return Operand::TempVar(temp);
            }
            case BuilderIR::InstructionBinaryOperation::Operation::Or: {
                TempVarID temp = allocateTempVar();
                LabelID keepChecking = allocateLabel();
                LabelID valueFalse = allocateLabel();
                LabelID valueTrue = allocateLabel();
                LabelID done = allocateLabel();
                emit(InstructionBranch(leftOperand, valueTrue, keepChecking));
                emit(InstructionLabel(keepChecking));
                emit(InstructionBranch(rightOperand, valueTrue, valueFalse));
                emit(InstructionLabel(valueFalse));
                emit(InstructionSet(temp, 0));
                emit(InstructionJump(done));
                emit(InstructionLabel(valueTrue));
                emit(InstructionSet(temp, 1));
                emit(InstructionLabel(done));
                return Operand::TempVar(temp);
            }
            case BuilderIR::InstructionBinaryOperation::Operation::Equals: {
                TempVarID temp = allocateTempVar();
                LabelID equal = allocateLabel();
                LabelID notEqual = allocateLabel();
                LabelID done = allocateLabel();
                emit(InstructionCompareEqual(leftOperand, rightOperand, equal, notEqual));
                emit(InstructionLabel(equal));
                emit(InstructionSet(temp, 1));
                emit(InstructionJump(done));
                emit(InstructionLabel(notEqual));
                emit(InstructionSet(temp, 0));
                emit(InstructionLabel(done));
                return Operand::TempVar(temp);
            }
            case BuilderIR::InstructionBinaryOperation::Operation::NotEquals: {
                TempVarID temp = allocateTempVar();
                LabelID notEqual = allocateLabel();
                LabelID equal = allocateLabel();
                LabelID done = allocateLabel();
                emit(InstructionCompareEqual(leftOperand, rightOperand, equal, notEqual));
                emit(InstructionLabel(equal));
                emit(InstructionSet(temp, 0));
                emit(InstructionJump(done));
                emit(InstructionLabel(notEqual));
                emit(InstructionSet(temp, 1));
                emit(InstructionLabel(done));
                return Operand::TempVar(temp);
            }
            case BuilderIR::InstructionBinaryOperation::Operation::GreaterEqual: {
                TempVarID temp = allocateTempVar();
                LabelID valueFalse = allocateLabel();
                LabelID valueTrue = allocateLabel();
                LabelID done = allocateLabel();
                emit(InstructionCompareLess(leftOperand, rightOperand, valueFalse, valueTrue));
                emit(InstructionLabel(valueFalse));
                emit(InstructionSet(temp, 0));
                emit(InstructionJump(done));
                emit(InstructionLabel(valueTrue));
                emit(InstructionSet(temp, 1));
                emit(InstructionLabel(done));
                return Operand::TempVar(temp);
            }
            case BuilderIR::InstructionBinaryOperation::Operation::GreaterThan: {
                TempVarID temp = allocateTempVar();
                LabelID valueTrue = allocateLabel();
                LabelID valueFalse = allocateLabel();
                LabelID done = allocateLabel();
                emit(InstructionCompareMore(leftOperand, rightOperand, valueTrue, valueFalse));
                emit(InstructionLabel(valueTrue));
                emit(InstructionSet(temp, 1));
                emit(InstructionJump(done));
                emit(InstructionLabel(valueFalse));
                emit(InstructionSet(temp, 0));
                emit(InstructionLabel(done));
                return Operand::TempVar(temp);
            }
            case BuilderIR::InstructionBinaryOperation::Operation::LessEqual: {
                TempVarID temp = allocateTempVar();
                LabelID valueFalse = allocateLabel();
                LabelID valueTrue = allocateLabel();
                LabelID done = allocateLabel();
                emit(InstructionCompareMore(leftOperand, rightOperand, valueFalse, valueTrue));
                emit(InstructionLabel(valueTrue));
                emit(InstructionSet(temp, 1));
                emit(InstructionJump(done));
                emit(InstructionLabel(valueFalse));
                emit(InstructionSet(temp, 0));
                emit(InstructionLabel(done));
                return Operand::TempVar(temp);
            }
            case BuilderIR::InstructionBinaryOperation::Operation::LessThan: {
                TempVarID temp = allocateTempVar();
                LabelID valueTrue = allocateLabel();
                LabelID valueFalse = allocateLabel();
                LabelID done = allocateLabel();
                emit(InstructionCompareLess(leftOperand, rightOperand, valueTrue, valueFalse));
                emit(InstructionLabel(valueFalse));
                emit(InstructionSet(temp, 0));
                emit(InstructionJump(done));
                emit(InstructionLabel(valueTrue));
                emit(InstructionSet(temp, 1));
                emit(InstructionLabel(done));
                return Operand::TempVar(temp);
            }
            default: {
                TempVarID temp = allocateTempVar();
                emit(InstructionBinaryOperation(temp, astBinopToIrBinop.at(binary->operation), leftOperand, rightOperand));
                return Operand::TempVar(temp);
            }
        }
    }

    throw std::runtime_error("Unrecognized expression, unable to lower");
}

inline static const std::unordered_map<AST::BinaryOperation::OperationType, BuilderIR::InstructionBranchCmp::ComparisonType> comparisonExpressions = {{
    {AST::BinaryOperation::OperationType::Equals, BuilderIR::InstructionBranchCmp::ComparisonType::Equals},
    {AST::BinaryOperation::OperationType::NotEquals, BuilderIR::InstructionBranchCmp::ComparisonType::NotEquals},
    {AST::BinaryOperation::OperationType::GreaterEqual, BuilderIR::InstructionBranchCmp::ComparisonType::GreaterEqual},
    {AST::BinaryOperation::OperationType::GreaterThan, BuilderIR::InstructionBranchCmp::ComparisonType::Greater},
    {AST::BinaryOperation::OperationType::LessEqual, BuilderIR::InstructionBranchCmp::ComparisonType::LessEqual},
    {AST::BinaryOperation::OperationType::LessThan, BuilderIR::InstructionBranchCmp::ComparisonType::Less}
}};

void BuilderIR::lowerStatement(const std::unique_ptr<AST::Statement> &statement)
{
    if(auto* letStatement = dynamic_cast<AST::VariableDeclaration*>(statement.get()))
    {
        Operand value = lowerExpression(letStatement->value);
        auto& variable = *dynamic_cast<AST::VariableData*>(letStatement);
        emit(InstructionStore(variable, value));
        return;
    }

    if(auto* assignStatement = dynamic_cast<AST::VariableAssignment*>(statement.get()))
    {
        Operand value = lowerExpression(assignStatement->value);
        auto& variable = *dynamic_cast<AST::VariableData*>(assignStatement);
        emit(InstructionStore(variable, value));
        return;
    }

    if(auto* displayStatement = dynamic_cast<AST::DisplayStatement*>(statement.get()))
    {
        Operand expression = lowerExpression(displayStatement->expression);
        emit(InstructionDisplay(expression));
        return;
    }

    if(auto* ifStatement = dynamic_cast<AST::IfStatement*>(statement.get()))
    {
        if(auto* binaryCondition = dynamic_cast<AST::BinaryOperation*>(ifStatement->condition.get()))
        {
            auto astOperation = binaryCondition->operation;
            if(comparisonExpressions.find(astOperation) != comparisonExpressions.end())
            {
                LabelID Lthen = allocateLabel();
                LabelID Lend = allocateLabel();

                Operand leftOperand = lowerExpression(binaryCondition->leftOperand);
                Operand rightOperand = lowerExpression(binaryCondition->rightOperand);

                auto operation = comparisonExpressions.at(astOperation);
                emit(InstructionBranchCmp(operation, leftOperand, rightOperand, Lthen, Lend));
                emit(InstructionLabel(Lthen));
                lowerStatement(ifStatement->body);
                emit(InstructionLabel(Lend));
                return;
            }
        }

        Operand condition = lowerExpression(ifStatement->condition);
        LabelID Lthen = allocateLabel();
        LabelID Lend = allocateLabel();

        emit(InstructionBranch(condition, Lthen, Lend));
        emit(InstructionLabel(Lthen));
        lowerStatement(ifStatement->body);
        emit(InstructionLabel(Lend));
        return;
    }

    if(auto* whileStatement = dynamic_cast<AST::WhileStatement*>(statement.get()))
    {
        if(auto* binaryCondition = dynamic_cast<AST::BinaryOperation*>(whileStatement->condition.get()))
        {
            auto astOperation = binaryCondition->operation;
            if(comparisonExpressions.find(astOperation) != comparisonExpressions.end())
            {
                LabelID Lcond = allocateLabel();
                LabelID Lbody = allocateLabel();
                LabelID Lend = allocateLabel();

                emit(InstructionLabel(Lcond));
                auto operation = comparisonExpressions.at(astOperation);
                Operand leftOperand = lowerExpression(binaryCondition->leftOperand);
                Operand rightOperand = lowerExpression(binaryCondition->rightOperand);
                emit(InstructionBranchCmp(operation, leftOperand, rightOperand, Lbody, Lend));

                emit(InstructionLabel(Lbody));
                lowerStatement(whileStatement->body);
                emit(InstructionJump(Lcond));

                emit(InstructionLabel(Lend));
                return;
            }
        }

        LabelID Lcond = allocateLabel();
        LabelID Lbody = allocateLabel();
        LabelID Lend = allocateLabel();

        emit(InstructionLabel(Lcond));
        Operand condition = lowerExpression(whileStatement->condition);
        emit(InstructionBranch(condition, Lbody, Lend));

        emit(InstructionLabel(Lbody));
        lowerStatement(whileStatement->body);
        emit(InstructionJump(Lcond));

        emit(InstructionLabel(Lend));
        return;
    }

    if(auto* codeBlock = dynamic_cast<AST::CodeBlock*>(statement.get()))
    {
        for(auto& innerStatement : codeBlock->block)
            lowerStatement(innerStatement);
        return;
    }

    throw std::runtime_error("Unrecognized statement, cannot lower");
}

void BuilderIR::lowerProgram(const std::vector<std::unique_ptr<AST::Statement>> &statements)
{
    code.clear();
    nextTemp = 0;
    nextLabel = 0;

    for(auto& statement : statements)
        lowerStatement(statement);
}

BuilderIR::TempVarID BuilderIR::getTempVarsCount() const
{
    return nextTemp;
}

BuilderIR::BuilderIR(const std::vector<std::unique_ptr<AST::Statement>> &program)
{
    lowerProgram(program);
    //tryOptimize();
}

const std::vector<BuilderIR::Instruction> &BuilderIR::getCode() const
{
    return code;
}

void BuilderIR::tryOptimize()
{
    for(auto it = code.begin(); it < code.end() - 1; ++it)
    {
        if(std::holds_alternative<BuilderIR::InstructionJump>(*it))
        {
            auto jumpIt = it;
            auto unreachable = std::next(jumpIt);
            auto scan = unreachable;

            while(scan != code.end() && !std::holds_alternative<InstructionLabel>(*scan))
                ++scan;
            
            if(scan != code.end() && unreachable != scan)
            {
                code.erase(unreachable, scan);
                it = std::prev(jumpIt);
            }
        }

        if(std::holds_alternative<BuilderIR::InstructionJump>(*it) && std::holds_alternative<BuilderIR::InstructionLabel>(*(it+1)))
        {
            auto& jump = std::get<BuilderIR::InstructionJump>(*it);
            auto& label = std::get<BuilderIR::InstructionLabel>(*(it+1));
            if(jump.destination != label.label) continue;
            
            it = code.erase(it);
            --it;
            continue;
        }

        if(std::holds_alternative<BuilderIR::InstructionLabel>(*it) && std::holds_alternative<BuilderIR::InstructionJump>(*(it+1)))
        {
            auto& label = std::get<BuilderIR::InstructionLabel>(*it);
            auto& jump = std::get<BuilderIR::InstructionJump>(*(it+1));

            if(label.label != jump.destination) continue;

            for(auto& instruction : code)
            {
                if(std::holds_alternative<BuilderIR::InstructionJump>(instruction))
                {
                    auto& _jump = std::get<BuilderIR::InstructionJump>(instruction);
                    if(_jump.destination == label.label)
                        _jump.destination = jump.destination;
                }
            }

            code.erase(it);
            it = code.begin();
            continue;
        }
    }
}

BuilderIR::TempVarID BuilderIR::allocateTempVar()
{
    return nextTemp++;
}

BuilderIR::LabelID BuilderIR::allocateLabel()
{
    return nextLabel++;
}

BuilderIR::InstructionBinaryOperation::InstructionBinaryOperation(TempVarID destination, const Operation &operation, const Operand &leftOperand, const Operand &rightOperand)
    : destination(destination), operation(operation), leftOperand(leftOperand), rightOperand(rightOperand) {}

BuilderIR::InstructionUnaryOperator::InstructionUnaryOperator(TempVarID destination, const Operand &operand, Operation operation)
    : destination(destination), operand(operand), operation(operation) {}

BuilderIR::InstructionLabel::InstructionLabel(LabelID label)
    : label(label) {}

BuilderIR::InstructionJump::InstructionJump(LabelID destination)
    : destination(destination) {}

BuilderIR::InstructionBranch::InstructionBranch(const Operand &condition, LabelID ifTrue, LabelID ifFalse)
    : condition(condition), ifTrue(ifTrue), ifFalse(ifFalse) {}

BuilderIR::InstructionSet::InstructionSet(TempVarID destination, int value)
    : destination(destination), value(value) {}

BuilderIR::InstructionCompareEqual::InstructionCompareEqual(const Operand &leftOperand, const Operand &rightOperand, LabelID ifEqual, LabelID ifNotEqual)
    : leftOperand(leftOperand), rightOperand(rightOperand), ifEqual(ifEqual), ifNotEqual(ifNotEqual) {}

BuilderIR::InstructionCompareLess::InstructionCompareLess(const Operand &leftOperand, const Operand &rightOperand, LabelID ifLess, LabelID ifMore)
    : leftOperand(leftOperand), rightOperand(rightOperand), ifLess(ifLess), ifMore(ifMore) {}

BuilderIR::InstructionCompareMore::InstructionCompareMore(const Operand &leftOperand, const Operand &rightOperand, LabelID ifMore, LabelID ifLess)
    : leftOperand(leftOperand), rightOperand(rightOperand), ifMore(ifMore), ifLess(ifLess) {}

BuilderIR::InstructionBranchCmp::InstructionBranchCmp(ComparisonType type, const Operand &leftOperand, const Operand &rightOperand, BuilderIR::LabelID ifTrue, BuilderIR::LabelID ifFalse)
    : type(type), leftOperand(leftOperand), rightOperand(rightOperand), ifTrue(ifTrue), ifFalse(ifFalse) {}

BuilderIR::InstructionLoad::InstructionLoad(TempVarID destination, const AST::VariableData &sourceVariable)
    : destination(destination), offset(sourceVariable.offset) {}

BuilderIR::InstructionStore::InstructionStore(const AST::VariableData &destinationVariable, const Operand &value)
    : offset(destinationVariable.offset), value(value) {}

BuilderIR::InstructionDisplay::InstructionDisplay(Operand operand)
    : operand(operand) {}
