#include "IR.hpp"

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
    {AST::BinaryOperation::OperationType::Division, BuilderIR::InstructionBinaryOperation::Operation::Division}
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
        emit(InstructionLoad(temp, identificator->identificator));
        return Operand::TempVar(temp);
    }

    if(auto* unary = dynamic_cast<AST::UnaryOperation*>(expression.get()))
    {
        auto operation = unary->operation;
        Operand a = lowerExpression(unary->operand);

        if(operation == AST::UnaryOperation::OperationType::Identity) return a;

        TempVarID temp = allocateTempVar();
        emit(InstructionNegation(temp, a));
        return Operand::TempVar(temp);
    }

    if(auto* binary = dynamic_cast<AST::BinaryOperation*>(expression.get()))
    {
        Operand leftOperand = lowerExpression(binary->leftOperand);
        Operand rightOperand = lowerExpression(binary->rightOperand);

        TempVarID temp = allocateTempVar();
        emit(InstructionBinaryOperation(temp, astBinopToIrBinop.at(binary->operation), leftOperand, rightOperand));
        return Operand::TempVar(temp);
    }

    throw std::runtime_error("Unrecognized expression, unable to lower");
}

void BuilderIR::lowerStatement(const std::unique_ptr<AST::Statement> &statement)
{
    if(auto* letStatement = dynamic_cast<AST::VariableDeclaration*>(statement.get()))
    {
        Operand value = lowerExpression(letStatement->value);
        emit(InstructionStore(letStatement->identificator, value));
        return;
    }

    if(auto* assignStatement = dynamic_cast<AST::VariableAssignment*>(statement.get()))
    {
        Operand value = lowerExpression(assignStatement->value);
        emit(InstructionStore(assignStatement->identificator, value));
        return;
    }

    if(auto* displayStatement = dynamic_cast<AST::DisplayStatement*>(statement.get()))
    {
        emit(InstructionDisplay(displayStatement->identificator));
        return;
    }

    if(auto* ifStatement = dynamic_cast<AST::IfStatement*>(statement.get()))
    {
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
}

const std::vector<BuilderIR::Instruction> &BuilderIR::getCode() const
{
    return code;
}

BuilderIR::TempVarID BuilderIR::allocateTempVar()
{
    return nextTemp++;
}

BuilderIR::LabelID BuilderIR::allocateLabel()
{
    return nextLabel++;
}

BuilderIR::InstructionLoad::InstructionLoad(TempVarID destination, std::string_view sourceSymbol)
    : destination(destination), sourceSymbol(sourceSymbol) {}

BuilderIR::InstructionStore::InstructionStore(std::string_view destinationSymbol, const Operand &value)
    : destinationSymbol(destinationSymbol), value(value) {}

BuilderIR::InstructionBinaryOperation::InstructionBinaryOperation(TempVarID destination, const Operation &operation, const Operand &leftOperand, const Operand &rightOperand)
    : destination(destination), operation(operation), leftOperand(leftOperand), rightOperand(rightOperand) {}

BuilderIR::InstructionNegation::InstructionNegation(TempVarID destination, const Operand &operand)
    : destination(destination), operand(operand) {}

BuilderIR::InstructionLabel::InstructionLabel(LabelID label)
    : label(label) {}

BuilderIR::InstructionJump::InstructionJump(LabelID destination)
    : destination(destination) {}

BuilderIR::InstructionBranch::InstructionBranch(const Operand &condition, LabelID ifTrue, LabelID ifFalse)
    : condition(condition), ifTrue(ifTrue), ifFalse(ifFalse) {}

BuilderIR::InstructionDisplay::InstructionDisplay(std::string_view symbol)
    : symbol(symbol) {}

std::ostream &operator<<(std::ostream &os, const BuilderIR::Operand &op)
{
    switch(op.type)
    {
        case BuilderIR::Operand::Type::Immediate:
            return os << op.immediate;
        case BuilderIR::Operand::Type::Temporary:
            return os << "t" << op.tempVar;
    }

    return os;
}

std::ostream &operator<<(std::ostream &os, const BuilderIR::InstructionLoad &i)
{
    return os << "\tLOAD t" << i.destination << ", [" << i.sourceSymbol << "]";
}

std::ostream &operator<<(std::ostream &os, const BuilderIR::InstructionStore &i)
{
    return os << "\tSTORE [" << i.destinationSymbol << "], " << i.value;
}

std::ostream &operator<<(std::ostream &os, const BuilderIR::InstructionBinaryOperation &i)
{
    switch(i.operation)
    {
        case BuilderIR::InstructionBinaryOperation::Operation::Addition: {
            os << "\tADD t";
        } break;
        case BuilderIR::InstructionBinaryOperation::Operation::Subtraction: {
            os << "\tSUB t";
        } break;
        case BuilderIR::InstructionBinaryOperation::Operation::Multiplication: {
            os << "\tMUL t";
        } break;
        case BuilderIR::InstructionBinaryOperation::Operation::Division: {
            os << "\tDIV t";
        } break;
    }

    return os << i.destination << ", " << i.leftOperand << ", " << i.rightOperand;
}

std::ostream &operator<<(std::ostream &os, const BuilderIR::InstructionNegation &i)
{
    return os << "\tNEG t" << i.destination << ", " << i.operand;
}

std::ostream &operator<<(std::ostream &os, const BuilderIR::InstructionLabel &i)
{
    return os << ".L" << i.label << ":";
}

std::ostream &operator<<(std::ostream &os, const BuilderIR::InstructionJump &i)
{
    return os << "\tJMP .L" << i.destination;
}

std::ostream &operator<<(std::ostream &os, const BuilderIR::InstructionBranch &i)
{
    return os << "\tBRNZ " << i.condition << ", .L" << i.ifTrue << ", .L" << i.ifFalse;
}

std::ostream &operator<<(std::ostream &os, const BuilderIR::InstructionDisplay &i)
{
    return os << "\tDISP " << i.symbol;
}
