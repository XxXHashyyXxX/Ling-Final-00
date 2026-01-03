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
    InstructionGenerator(std::ostream& os) : os(os) {}

    void operator()(BuilderIR::InstructionLoad load) const;
    void operator()(BuilderIR::InstructionStore store) const;
    void operator()(BuilderIR::InstructionBinaryOperation binaryOperation) const;
    void operator()(BuilderIR::InstructionUnaryOperator negation) const;
    void operator()(BuilderIR::InstructionLabel label) const;
    void operator()(BuilderIR::InstructionJump jump) const;
    void operator()(BuilderIR::InstructionBranch branch) const;
    void operator()(BuilderIR::InstructionDisplay display) const;
    void operator()(BuilderIR::InstructionSet set) const;
    void operator()(BuilderIR::InstructionCompareEqual cmpEqual) const;
    void operator()(BuilderIR::InstructionCompareLess cmpLess) const;
    void operator()(BuilderIR::InstructionCompareMore cmpMore) const;

    std::ostream& os;
};

inline static std::string generateMov(const std::string& to, const std::string& from)
{
    return "\tmov " + to + ", " + from + "\n";
}

inline static std::string generateMovImmediate(const std::string& to, int immediate)
{
    std::ostringstream immediateString;
    immediateString << immediate;
    return generateMov(to, immediateString.str());
}

inline static std::string generateMovImmediateToMemory(const std::string& to, int immediate)
{
    return generateMovImmediate("qword [" + to + "]", immediate);
}

inline static std::string generateMovImmediateToTempVar(BuilderIR::TempVarID to, int immediate)
{
    std::ostringstream _to;
    _to << to;
    return generateMovImmediateToMemory("__tempVar__t" + _to.str(), immediate);
}

inline static std::string generateMovFromMemory(const std::string& toRegister, const std::string& from)
{
    return generateMov(toRegister, "qword [" + from + "]");
}

inline static std::string generateMovFromTempVar(const std::string& toRegister, BuilderIR::TempVarID from)
{
    std::ostringstream _from;
    _from << "__tempVar__t" << from;
    return generateMovFromMemory(toRegister, _from.str());
}

inline static std::string generateMovToMemory(const std::string& to, const std::string& fromRegister)
{
    return generateMov("qword [" + to + "]", fromRegister);
}

inline static std::string generateMovToTempVar(BuilderIR::TempVarID to, const std::string& fromRegister)
{
    std::ostringstream _to;
    _to << "__tempVar__t" << to;
    return generateMovToMemory(_to.str(), fromRegister);
}

std::string CodeGen::generateAssembly(std::string_view fileName, const BuilderIR& builderIR, const SymbolTable &symbolTable)
{
    std::ofstream displayFunctionAssemblyCode("__display__function__.asm");
    displayFunctionAssemblyCode << displayFunctionAssembly;
    displayFunctionAssemblyCode.close();

    std::system(("rm -f " + std::string(fileName) + ".asm " + std::string(fileName)).c_str());
    std::ofstream assemblyCode(std::string(fileName) + ".asm");

    // set relative mode
    assemblyCode << "default rel\n\n";
    
    // begin bss section
    assemblyCode << "section .bss\n";
    
    // add bss symbol for each variable
    for(auto& symbol : symbolTable)
    {
        assemblyCode << "\t" << symbol << "\tresq\t1\n";
    }
    
    // add bss symbol for each temp var
    for(auto i = 0; i < builderIR.getTempVarsCount(); ++i)
    {
        assemblyCode << "\t__tempVar__t" << i << "\tresq\t1\n";
    }

    assemblyCode << "\n";

    // add text section
    assemblyCode << "section .text\n"
                    "\tglobal _start\n"
                    "\textern __display__function__\n"
                    "\n_start:\n";
    
    // generate code for each instruction
    auto code = builderIR.getCode();
    for(auto& instruction : code)
    {
        std::visit(InstructionGenerator(assemblyCode), instruction);
    }

    // generate epilogue
    assemblyCode << "\tmov rax, 60\n"
                    "\txor rdi, rdi\n"
                    "\tsyscall\n";

    assemblyCode.close();
    return std::string(fileName) + ".asm";
}

std::string CodeGen::generateObjectFile(const std::string& asmFile, std::string_view fileName)
{
    std::string objectFile = std::string(fileName) + ".o";
    std::system((std::string("nasm -f elf64 ") + asmFile + " -o " + objectFile).c_str());
    std::system("nasm -f elf64 __display__function__.asm -o __display__function__.o");
    return objectFile;
}

void CodeGen::linkObjectFile(const std::string& objectFile, std::string_view fileName)
{
    std::system(("ld " + objectFile + " __display__function__.o -o " + std::string(fileName)).c_str());
}

void CodeGen::generateCode(std::string_view fileName, const BuilderIR& builderIR, const SymbolTable &symbolTable)
{
    auto assemblyCode = generateAssembly(fileName, builderIR, symbolTable);
    auto objectFile = generateObjectFile(assemblyCode, fileName);
    linkObjectFile(objectFile, fileName);

    std::system((std::string("rm ") + assemblyCode + " " + objectFile + " __display__function__.asm __display__function__.o").c_str());
}

void InstructionGenerator::operator()(BuilderIR::InstructionLoad load) const
{
    os << generateMovFromMemory("rax", load.sourceSymbol);
    os << generateMovToTempVar(load.destination, "rax");
}

void InstructionGenerator::operator()(BuilderIR::InstructionStore store) const
{
    switch(store.value.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediateToMemory(store.destinationSymbol, store.value.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rax", store.value.tempVar);
            os << generateMovToMemory(store.destinationSymbol, "rax");
        } break;
    }
}

void InstructionGenerator::operator()(BuilderIR::InstructionBinaryOperation binaryOperation) const
{
    switch(binaryOperation.leftOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rax", binaryOperation.leftOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rax", binaryOperation.leftOperand.tempVar);
        } break;
    }

    switch(binaryOperation.rightOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rbx", binaryOperation.rightOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rbx", binaryOperation.rightOperand.tempVar);
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

void InstructionGenerator::operator()(BuilderIR::InstructionUnaryOperator unary) const
{
    switch(unary.operand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rax", unary.operand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rax", unary.operand.tempVar);
        } break;
    }

    switch(unary.operation)
    {
        case BuilderIR::InstructionUnaryOperator::Operation::Negation: {
            os << "\tneg rax\n";
        } break;
        case BuilderIR::InstructionUnaryOperator::Operation::Not: {
            os << "\tnot rax\n";
        } break;
    }

    os << generateMovToTempVar(unary.destination, "rax");
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
    switch(branch.condition.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rax", branch.condition.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rax", branch.condition.tempVar);
        } break;
    }

    os << "\ttest rax, rax\n";
    os << "\tjnz .L" << branch.ifTrue << "\n";
    os << "\tjmp .L" << branch.ifFalse << "\n";
}

void InstructionGenerator::operator()(BuilderIR::InstructionDisplay display) const
{
    os << generateMovFromMemory("rdi", display.symbol);
    os << "\tcall __display__function__\n";
}

void InstructionGenerator::operator()(BuilderIR::InstructionSet set) const
{
    os << generateMovImmediate("rax", set.value);
    os << generateMovToTempVar(set.destination, "rax");
}

void InstructionGenerator::operator()(BuilderIR::InstructionCompareEqual cmpEqual) const
{
    switch(cmpEqual.leftOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rax", cmpEqual.leftOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rax", cmpEqual.leftOperand.tempVar);
        }
    }

    switch(cmpEqual.rightOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rbx", cmpEqual.rightOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rbx", cmpEqual.rightOperand.tempVar);
        } break;
    }

    os << "\tcmp rax, rbx\n";
    os << "\tje .L" << cmpEqual.ifEqual << "\n";
    os << "\tjmp .L" << cmpEqual.ifNotEqual << "\n";
}

void InstructionGenerator::operator()(BuilderIR::InstructionCompareLess cmpLess) const
{
    switch(cmpLess.leftOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rax", cmpLess.leftOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rax", cmpLess.leftOperand.tempVar);
        }
    }

    switch(cmpLess.rightOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rbx", cmpLess.rightOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rbx", cmpLess.rightOperand.tempVar);
        } break;
    }

    os << "\tcmp rax, rbx\n";
    os << "\tjl .L" << cmpLess.ifLess << "\n";
    os << "\tjmp .L" << cmpLess.ifMore << "\n";
}

void InstructionGenerator::operator()(BuilderIR::InstructionCompareMore cmpMore) const
{
    switch(cmpMore.leftOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rax", cmpMore.leftOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rax", cmpMore.leftOperand.tempVar);
        }
    }

    switch(cmpMore.rightOperand.type)
    {
        case BuilderIR::Operand::Type::Immediate: {
            os << generateMovImmediate("rbx", cmpMore.rightOperand.immediate);
        } break;
        case BuilderIR::Operand::Type::Temporary: {
            os << generateMovFromTempVar("rbx", cmpMore.rightOperand.tempVar);
        } break;
    }

    os << "\tcmp rax, rbx\n";
    os << "\tjg .L" << cmpMore.ifMore << "\n";
    os << "\tjmp .L" << cmpMore.ifLess << "\n";
}
