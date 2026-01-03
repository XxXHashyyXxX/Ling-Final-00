#include "Tokens.hpp"
#include <stdexcept>
#include <cctype>
#include <unordered_map>
#include <optional>
#include <iostream>

using namespace Tokenization;

std::unordered_map<std::string, Token::Type> types = {{
    {"=", Token::Type::OperatorAssign},
    {"+", Token::Type::OperatorPlus},
    {"-", Token::Type::OperatorMinus},
    {"*", Token::Type::OperatorStar},
    {"/", Token::Type::OperatorSlash},
    {"%", Token::Type::OperatorPercent},
    {"(", Token::Type::ParenthesisLeft},
    {")", Token::Type::ParenthesisRight},
    {"if", Token::Type::KeywordIf},
    {"let", Token::Type::KeywordLet},
    {"while", Token::Type::KeywordWhile},
    {"display", Token::Type::KeywordDisplay},
    {";", Token::Type::EndOfLine},
    {"{", Token::Type::BraceLeft},
    {"}", Token::Type::BraceRight}
}};

static bool matchKeyword(const char* it, const char* end, const std::string& key) {
    for(auto c : key) {
        if(it == end || *it != c) return false;
        ++it;
    }
    return true;
}

static std::optional<Token::Type> matchLongestTokenType(const char* &it, const char* end) {
    struct Match {
        Token::Type type;
        size_t length;

        Match(Token::Type type, size_t length) : type(type), length(length) {}
    };

    std::optional<Match> best;

    for(const auto& [key, type] : types) {
        if(matchKeyword(it, end, key)) {
            if(!best.has_value() || key.size() > best->length) {
                best = Match(type, key.size());
            }
        }
    }

    if(!best.has_value()) return std::optional<Token::Type>();
    
    it += best->length;
    return best->type;
}

std::vector<Token> Tokenization::tokenize(std::string_view source)
{
    std::vector<Token> out;
    auto sourceEnd = source.end();

    for(auto it = source.begin(); it != sourceEnd; ++it) {
        if(std::isspace(*it)) continue;
        if(*it == '\0') break;

        if(std::isdigit(*it)) {
            auto start = it;
            while(it != sourceEnd && std::isdigit(*it)) ++it;
            auto end = it--;

            out.emplace_back(Token::Type::Literal, std::string(start, end));
            continue;
        }

        auto tokenType = matchLongestTokenType(it, source.end());
        if(tokenType) {
            --it;
            out.emplace_back(*tokenType, "");
            continue;
        }

        if(std::isalpha(*it)) {
            auto start = it;
            while(it != sourceEnd && (std::isalnum(*it) || *it == '_')) ++it;
            auto end = it--;

            out.emplace_back(Token::Type::Identificator, std::string(start, end));
            continue;
        }

        std::cerr << "Troublesome character: " << static_cast<int>(*it) << "\nAt position " << (it - source.begin()) << "\n";
        throw std::runtime_error("Token couldnt be matched");
    }
    return out;
}

std::ostream &Tokenization::operator<<(std::ostream &os, const Token &token)
{
    os << token.type;
    if(token.type == Token::Type::Identificator || token.type == Token::Type::Literal) os << token.value;
    return os;
}

std::ostream &Tokenization::operator<<(std::ostream &os, const Token::Type &type)
{
    switch(type)
    {
        case Token::Type::EndOfLine:
            return os << ';';
        case Token::Type::Identificator:
            return os << "Identificator: ";
        case Token::Type::KeywordIf:
            return os << "if";
        case Token::Type::KeywordLet:
            return os << "let";
        case Token::Type::KeywordWhile:
            return os << "while";
        case Token::Type::KeywordDisplay:
            return os << "display";
        case Token::Type::Literal:
            return os << "literal: ";
        case Token::Type::OperatorAssign:
            return os << '=';
        case Token::Type::OperatorMinus:
            return os << '-';
        case Token::Type::OperatorPlus:
            return os << '+';
        case Token::Type::OperatorSlash:
            return os << '/';
        case Token::Type::OperatorStar:
            return os << '*';
        case Token::Type::OperatorPercent:
            return os << "%";
        case Token::Type::ParenthesisLeft:
            return os << '(';
        case Token::Type::ParenthesisRight:
            return os << ')';
        case Token::Type::BraceLeft:
            return os << "{";
        case Token::Type::BraceRight:
            return os << "}";
        default:
            throw std::runtime_error("Invalid type");
    }
}

Tokenization::Token::Token(Type type, const std::string &value) : type(type), value(value)
{
    if(value.empty() && (type == Token::Type::Identificator || type == Token::Type::Literal))
    {
        std::cerr << "Token type: " << type << "\nValue: " << value << "\n";
        throw std::invalid_argument("Cannot create empty token of provided type");
    }
}