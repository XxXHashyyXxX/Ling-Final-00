#pragma once

#include <string>
#include <vector>
#include <ostream>

namespace Tokenization {
    struct Token {
        enum class Type {
            Literal,
            Identificator,
            OperatorPlus,
            OperatorMinus,
            OperatorStar,
            OperatorSlash,
            OperatorAssign,
            ParenthesisLeft,
            ParenthesisRight,
            KeywordLet,
            KeywordIf,
            KeywordWhile,
            EndOfLine
        } type;

        std::string value;

        Token(Type type, const std::string& value = "");
    };

    std::vector<Token> tokenize(const std::string& source);

    std::ostream& operator<<(std::ostream& os, const Token& token);
    std::ostream& operator<<(std::ostream& os, const Token::Type& type);
};