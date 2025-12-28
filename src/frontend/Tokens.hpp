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
            KeywordDisplay,
            EndOfLine
        } type;

        std::string value;

        Token(Type type, const std::string& value = "");
    };

    std::vector<Token> tokenize(std::string_view source);

    std::ostream& operator<<(std::ostream& os, const Token& token);
    std::ostream& operator<<(std::ostream& os, const Token::Type& type);
};