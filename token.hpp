#pragma once

#include <stdexcept>
#include <string_view>
#include <variant>
#include <vector>

namespace tonal {
using namespace std;

enum class TokenType : char {
  LIST,
  OPERATOR,
  KEYWORD,
  IDENTIFIER,
  NUMBER,
  STRING,
  WHITESPACE
};

#undef FALSE
#undef INFINITY
#undef NAN
#undef NULL
#undef TRUE

enum class Keyword : char {
  /**
   * Declaration keywords
   */
  MODULE,
  CONCEPT,
  CLASS,
  FUNCTION,
  SCOPE,
  LABEL,
  READABLE,
  WRITABLE,
  MUTABLE,
  NEW,
  LIST,
  CAST,
  DOC,
  /**
   * Flow control keywords
   */
  IF,
  SWITCH,
  CASE,
  WHILE,
  FOR,
  DO,
  BREAK,
  CONTINUE,
  GOTO,
  RETURN,
  YIELD,
  THROW,
  TRY,
  CATCH,
  MAIN,
  /**
   * Special value keywords
   */
  ANY,
  DELETE,
  DEFAULT,
  VOID,
  NULL,
  TRUE,
  FALSE,
  MIN,
  MAX,
  INFINITY,
  NAN,
  EPSILON,
  THIS,
  THIS_SCOPE,
  THIS_FUNCTION,
  THIS_CLASS,
  THIS_TEMPLATE,
  THIS_CONCEPT,
  THIS_MODULE,
  THIS_LIST,
  THIS_FILE,
  THIS_LINE,
  THIS_COLUMN,
  THIS_BYTE,
};

class Token {
public:
  Token() = default;
  Token(string_view v);

  int pos, token, line, column, paren, indent;
  string_view region;

  struct List {
    bool end = false;
    bool unpack = false;
  };

  struct Operator {
    string_view op;
  };

  struct Keyword {
    tonal::Keyword keyword;
  };

  struct Identifier {
    string_view id;
    vector<string_view> qualified;
    bool pack = false;
    bool unpack = false;
  };

  struct Number {
    string_view sign;
    string_view base;
    string_view numerator;
    string_view decimal_point;
    string_view denominator;
    string_view exponent_point;
    string_view exponent_sign;
    string_view exponent;
  };

  struct String {
    string_view encoding;
    string_view begin_quote;
    string_view begin_delimiter;
    string_view characters;
    string_view end_delimiter;
    string_view end_quote;
  };

  struct Whitespace {};

  variant<List, Operator, Keyword, Identifier, Number, String, Whitespace>
      detail = Whitespace{};
};

ostream &operator<<(ostream &, const Token &token);

vector<Token> tokenize(string_view tokens);
} // namespace tonal

/**
 *                   +-> Identifiers -> Scope
 *                   |      ^
 * File -> Tokens -> |      |
 *                   |      v        +-> Declarations -> Overloads
 *                   +-> Entities -> |
 *                                   +-> Descriptions -> Reifications
 */
