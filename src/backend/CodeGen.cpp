#include "CodeGen.hpp"
#include <cstdlib>
#include <fstream>
#include <sstream>

const char* displayFunctionAssembly = R"(default rel

section .bss
    __display__buffer__     resb    32

section .text
    global __display__function__

__display__function__:
    mov rax, rdi
    lea rsi, [__display__buffer__ + 31]
    mov byte [rsi], 10
    mov rcx, 1

    test rax, rax
    jns .positive

    neg rax
    mov r8b, '-'
    jmp .convert

.positive:
    xor r8b, r8b

.convert:
    cmp rax, 0
    jne .convert_loop

    dec rsi
    mov byte [rsi], '0'
    inc rcx
    jmp .sign

.convert_loop:
    mov r9, 10
.loop_div:
    xor rdx, rdx
    div r9
    add dl, '0'
    dec rsi
    mov [rsi], dl
    inc rcx
    test rax, rax
    jnz .loop_div

.sign:
    test r8b, r8b
    jz .write
    dec rsi
    mov [rsi], r8b
    inc rcx

.write:
    mov rax, 1
    mov rdi, 1
    mov rdx, rcx
    syscall
    ret)";

struct InstructionGenerator {
    InstructionGenerator(std::ostream& os, const BuilderIR& builderIR, const SymbolTable& symbolTable) 
        : os(os), builderIR(builderIR), symbolTable(symbolTable), localVariablesOffset(symbolTable.getOffset()) {}

    void operator()(BuilderIR::InstructionLoad load) const;
    void operator()(BuilderIR::InstructionStore store) const;
    void operator()(BuilderIR::InstructionBinaryOperation binaryOperation) const;
    void operator()(BuilderIR::InstructionUnaryOperator unaryOperation) const;
    void operator()(BuilderIR::InstructionLabel label) const;
    void operator()(BuilderIR::InstructionJump jump) const;
    void operator()(BuilderIR::InstructionBranch branch) const;
    void operator()(BuilderIR::InstructionDisplay display) const;
    void operator()(BuilderIR::InstructionSet set) const;
    void operator()(BuilderIR::InstructionCompareEqual cmpEqual) const;
    void operator()(BuilderIR::InstructionCompareLess cmpLess) const;
    void operator()(BuilderIR::InstructionCompareMore cmpMore) const;
    void operator()(BuilderIR::InstructionBranchCmp branchCmp) const;

    std::ostream& os;
    const BuilderIR& builderIR;
    const SymbolTable& symbolTable;
    const unsigned localVariablesOffset;

    unsigned getTempVarOffset(BuilderIR::TempVarID temp) const;

    std::string getAddresFromOffset(unsigned offset) const;
    std::string getTempVarAddress(BuilderIR::TempVarID temp) const;

    std::string convertAddressToValue(const std::string& address) const;

    std::string generateMovImmediate(const std::string& to, int immediate) const;
    std::string generateMovImmediateToTempVar(BuilderIR::TempVarID temp, int immediate) const;
    std::string generateMovImmediateToLocalVar(unsigned variableOffset, int immediate) const;
    std::string generateMovToTempVar(BuilderIR::TempVarID temp, const std::string& from) const;
    std::string generateMovToLocalVar(unsigned variableOffset, const std::string& from) const;
    std::string generateMovFromTempVar(const std::string& to, BuilderIR::TempVarID temp) const;
    std::string generateMovFromLocalVar(const std::string& to, unsigned variableOffset) const;
};

inline static std::string generateMov(const std::string& to, const std::string& from)
{
    return "\tmov " + to + ", " + from + "\n";
}

CodeGen::CodeGen(const BuilderIR &builderIR, const SymbolTable &symbolTable)
    : builderIR(builderIR), symbolTable(symbolTable) {}

std::string CodeGen::generateAssembly(const std::string &name)
{
    std::ofstream code(name + ".asm");

    // set default mode to relative
    code << "default rel\n";

    // add .text section
    code << "section .text\n"
            "\tglobal _start\n"
            "\textern __display__function__\n";

    // add _start prologue
    code << "_start:\n"
            "\tpush rbp\n"
            "\tmov rbp, rsp\n"
            "\tsub rsp, " << 8 * builderIR.getTempVarsCount() + symbolTable.getOffset() << "\n";
    
    // generate code
    auto& instructions = builderIR.getCode();
    InstructionGenerator generator(code, builderIR, symbolTable);
    
    for(auto& instruction : instructions)
    {
        std::visit(generator, instruction);
    }
    
    // add _start epilogue
    code << "\tmov rsp, rbp\n"
            "\tpop rbp\n";

    // exit
    code << "\tmov rax, 60\n"
            "\txor rdi, rdi\n"
            "\tsyscall";

    code.close();
    return name + ".asm";
}

std::string CodeGen::generateObjectFile(const std::string &name)
{
    std::system(("nasm -f elf64 " + name + ".asm -o " + name + ".o").c_str());
    return name + ".o";
}

void CodeGen::linkExecutable(const std::string &name)
{
    std::ofstream displayFunctionAssemblyCode("__display__function__.asm");
    displayFunctionAssemblyCode << displayFunctionAssembly;
    displayFunctionAssemblyCode.close();
    std::system("nasm -f elf64 __display__function__.asm -o __display__function__.o");
    std::system(("ld __display__function__.o " + name + ".o -o " + name).c_str());
    std::system("rm -f __display__function__.asm __display__function__.o");
}

void CodeGen::generateExecutable(const std::string &name)
{
    auto assemblyName = generateAssembly(name);
    auto objectName = generateObjectFile(name);
    linkExecutable(name);

    std::system(("rm -f " + assemblyName + " " + objectName).c_str());
}

unsigned InstructionGenerator::getTempVarOffset(BuilderIR::TempVarID temp) const
{
    return localVariablesOffset + temp * 8;
}

std::string InstructionGenerator::getAddresFromOffset(unsigned offset) const
{
    std::ostringstream oss;
    oss << "rbp-" << offset;
    return oss.str();
}

std::string InstructionGenerator::getTempVarAddress(BuilderIR::TempVarID temp) const
{
    return getAddresFromOffset(getTempVarOffset(temp));
}

std::string InstructionGenerator::convertAddressToValue(const std::string &address) const
{
    return "qword [" + address + "]";
}

std::string InstructionGenerator::generateMovImmediate(const std::string &to, int immediate) const
{
    std::ostringstream oss;
    oss << immediate;
    return generateMov(to, oss.str());
}

std::string InstructionGenerator::generateMovImmediateToTempVar(BuilderIR::TempVarID temp, int immediate) const
{
    std::ostringstream oss;
    oss << immediate;
    return generateMovToTempVar(temp, oss.str());
}

std::string InstructionGenerator::generateMovToTempVar(BuilderIR::TempVarID temp, const std::string &from) const
{
    auto destination = convertAddressToValue(getTempVarAddress(temp));
    return generateMov(destination, from);
}

std::string InstructionGenerator::generateMovFromTempVar(const std::string &to, BuilderIR::TempVarID temp) const
{
    auto source = convertAddressToValue(getTempVarAddress(temp));
    return generateMov(to, source);
}

std::string InstructionGenerator::generateMovImmediateToLocalVar(unsigned variableOffset, int immediate) const
{
    std::ostringstream oss;
    oss << immediate;
    return generateMovToLocalVar(variableOffset, oss.str());
}

std::string InstructionGenerator::generateMovToLocalVar(unsigned variableOffset, const std::string& from) const
{
    auto destination = convertAddressToValue(getAddresFromOffset(variableOffset));
    return generateMov(destination, from);
}

std::string InstructionGenerator::generateMovFromLocalVar(const std::string& to, unsigned variableOffset) const
{
    auto source = convertAddressToValue(getAddresFromOffset(variableOffset));
    return generateMov(to, source);
}

void InstructionGenerator::operator()(BuilderIR::InstructionLoad load) const
{
    os << generateMovFromLocalVar("rax", load.offset);
    os << generateMovToTempVar(load.destination, "rax");
}

void InstructionGenerator::operator()(BuilderIR::InstructionStore store) const
{
    switch(store.value.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediateToLocalVar(store.offset, store.value.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rax", store.value.tempVar);
            os << generateMovToLocalVar(store.offset, "rax");
        } break;
    }
}

void InstructionGenerator::operator()(BuilderIR::InstructionBinaryOperation binaryOperation) const
{
    auto leftOperand = binaryOperation.leftOperand;
    auto rightOperand = binaryOperation.rightOperand;

    switch(leftOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rax", leftOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rax", leftOperand.tempVar);
        } break;
    }

    switch(rightOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rbx", rightOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rbx", rightOperand.tempVar);   
        } break;
    }

    switch(binaryOperation.operation)
    {
        case BuilderIR::InstructionBinaryOperation::Operation::Addition: {
            os << "\tadd rax, rbx\n";
        } break;
        case BuilderIR::InstructionBinaryOperation::Operation::Subtraction: {
            os << "\tsub rax, rbx\n";
        } break;
        case BuilderIR::InstructionBinaryOperation::Operation::Multiplication: {
            os << "\tmul rax, rbx\n";
        } break;
        case BuilderIR::InstructionBinaryOperation::Operation::Division: {
            os << "\txor rdx, rdx\n";
            os << "\tdiv rbx\n";
        } break;
        case BuilderIR::InstructionBinaryOperation::Operation::Modulo: {
            os << "\txor rdx, rdx\n";
            os << "\tdiv rbx\n";
            os << generateMovToTempVar(binaryOperation.destination, "rdx");
            return;
        } break;
        default: {
            return;
        }
    }

    os << generateMovToTempVar(binaryOperation.destination, "rax");
}

void InstructionGenerator::operator()(BuilderIR::InstructionUnaryOperator unaryOperation) const
{
    auto operand = unaryOperation.operand;

    switch(operand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rax", operand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rax", operand.tempVar);
        } break;
    }

    switch(unaryOperation.operation)
    {
        case BuilderIR::InstructionUnaryOperator::Operation::Negation: {
            os << "\tneg rax\n";
        } break;
    }

    os << generateMovToTempVar(unaryOperation.destination, "rax");
}

void InstructionGenerator::operator()(BuilderIR::InstructionLabel label) const
{
    os << ".L" << label.label << ":\n";
}

void InstructionGenerator::operator()(BuilderIR::InstructionJump jump) const
{
    os << "\tjmp .L" << jump.destination << "\n";
}

void InstructionGenerator::operator()(BuilderIR::InstructionBranch branch) const
{
    auto condition = branch.condition;
    
    switch(condition.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rax", condition.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rax", condition.tempVar);
        } break;
    }

    os <<   "\ttest rax, rax\n"
            "\tjnz .L" << branch.ifTrue << "\n"
            "\tjmp .L" << branch.ifFalse << "\n";
}

void InstructionGenerator::operator()(BuilderIR::InstructionDisplay display) const
{
    auto operand = display.operand;

    switch(operand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rdi", operand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rdi", operand.tempVar);
        } break;
    }

    os << "\tcall __display__function__\n";
}

void InstructionGenerator::operator()(BuilderIR::InstructionSet set) const
{
    os << generateMovImmediateToTempVar(set.destination, set.value);
}

void InstructionGenerator::operator()(BuilderIR::InstructionCompareEqual cmpEqual) const
{
    auto leftOperand = cmpEqual.leftOperand;
    auto rightOperand = cmpEqual.rightOperand;

    switch(leftOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rax", leftOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rax", leftOperand.tempVar);
        } break;
    }

    switch(rightOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rbx", rightOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rbx", rightOperand.tempVar);
        } break;
    }

    os <<   "\tcmp rax, rbx\n"
            "\tje .L" << cmpEqual.ifEqual << "\n"
            "\tjmp .L" << cmpEqual.ifNotEqual << "\n";
}

void InstructionGenerator::operator()(BuilderIR::InstructionCompareLess cmpLess) const
{
    auto leftOperand = cmpLess.leftOperand;
    auto rightOperand = cmpLess.rightOperand;

    switch(leftOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rax", leftOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rax", leftOperand.tempVar);
        } break;
    }

    switch(rightOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rbx", rightOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rbx", rightOperand.tempVar);
        } break;
    }

    os <<   "\tcmp rax, rbx\n"
            "\tjl .L" << cmpLess.ifLess << "\n"
            "\tjmp .L" << cmpLess.ifMore << "\n";
}

void InstructionGenerator::operator()(BuilderIR::InstructionCompareMore cmpMore) const
{
    auto leftOperand = cmpMore.leftOperand;
    auto rightOperand = cmpMore.rightOperand;

    switch(leftOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rax", leftOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rax", leftOperand.tempVar);
        } break;
    }

    switch(rightOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rbx", rightOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rbx", rightOperand.tempVar);
        } break;
    }

    os <<   "\tcmp rax, rbx\n"
            "\tjg .L" << cmpMore.ifMore << "\n"
            "\tjmp .L" << cmpMore.ifLess << "\n";
}

void InstructionGenerator::operator()(BuilderIR::InstructionBranchCmp branchCmp) const
{
    auto leftOperand = branchCmp.leftOperand;
    auto rightOperand = branchCmp.rightOperand;

    switch(leftOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rax", leftOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rax", leftOperand.tempVar);
        } break;
    }

    switch(rightOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rbx", rightOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rbx", rightOperand.tempVar);
        } break;
    }

    os << "\tcmp rax, rbx\n";

    switch(branchCmp.type)
    {
        case BuilderIR::InstructionBranchCmp::ComparisonType::Equals: {
            os << "\tje";
        } break;
        case BuilderIR::InstructionBranchCmp::ComparisonType::NotEquals: {
            os << "\tjne";
        } break;
        case BuilderIR::InstructionBranchCmp::ComparisonType::Greater: {
            os << "\tjg";
        } break;
        case BuilderIR::InstructionBranchCmp::ComparisonType::GreaterEqual: {
            os << "\tjge";
        } break;
        case BuilderIR::InstructionBranchCmp::ComparisonType::Less: {
            os << "\tjl";
        } break;
        case BuilderIR::InstructionBranchCmp::ComparisonType::LessEqual: {
            os << "\tjle";
        } break;
    }

    os <<   " .L" << branchCmp.ifTrue << "\n"
            "\tjmp .L" << branchCmp.ifFalse << "\n";
}
