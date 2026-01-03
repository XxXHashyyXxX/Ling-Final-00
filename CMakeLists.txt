cmake_minimum_required(VERSION 3.14)
project(ling)

set(CMAKE_CXX_STANDARD 20)

add_executable(ling src/main.cpp
                    src/frontend/Tokens.cpp
                    src/frontend/AST.cpp
                    src/frontend/Parser.cpp
                    src/backend/SymbolTable.cpp
                    src/backend/IR.cpp
                    src/backend/CodeGen.cpp)