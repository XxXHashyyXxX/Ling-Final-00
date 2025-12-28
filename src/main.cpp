#include <iostream>
#include "frontend/Tokens.hpp"

using namespace Tokenization;

int main() {
    auto out = tokenize("let x = 3;");
    for(const auto& token : out) {
        std::cout << token << std::endl;
    }
    return 0;
}