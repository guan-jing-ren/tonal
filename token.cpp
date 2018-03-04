#include "token.hpp"

#include <iostream>
#include <numeric>
#include <regex>
#include <typeinfo>
#include <unordered_map>

#undef FALSE
#undef INFINITY
#undef NAN
#undef NULL
#undef TRUE

using namespace std;
using namespace tonal;

struct COUT {
} out;

template <typename T> COUT operator<<(COUT, const T &t) {
  //   cout << t;
  return {};
}

static const unordered_map<string_view, Keyword> identifier2keyword = {
    /**
     * Declaration keywords
     */
    {"module", Keyword::MODULE},
    {"concept", Keyword::CONCEPT},
    {"class", Keyword::CLASS},
    {"function", Keyword::FUNCTION},
    {"scope", Keyword::SCOPE},
    {"label", Keyword::LABEL},
    {"readable", Keyword::READABLE},
    {"writable", Keyword::WRITABLE},
    {"mutable", Keyword::MUTABLE},
    {"new", Keyword::NEW},
    {"list", Keyword::LIST},
    {"cast", Keyword::CAST},
    {"doc", Keyword::DOC},
    /**
     * Flow control keywords
     */
    {"if", Keyword::IF},
    {"switch", Keyword::SWITCH},
    {"case", Keyword::CASE},
    {"while", Keyword::WHILE},
    {"for", Keyword::FOR},
    {"do", Keyword::DO},
    {"break", Keyword::BREAK},
    {"continue", Keyword::CONTINUE},
    {"goto", Keyword::GOTO},
    {"return", Keyword::RETURN},
    {"yield", Keyword::YIELD},
    {"throw", Keyword::THROW},
    {"try", Keyword::TRY},
    {"catch", Keyword::CATCH},
    {"main", Keyword::MAIN},
    /**
     * Special value keywords
     */
    {"any", Keyword::ANY},
    {"delete", Keyword::DELETE},
    {"default", Keyword::DEFAULT},
    {"void", Keyword::VOID},
    {"null", Keyword::NULL},
    {"true", Keyword::TRUE},
    {"false", Keyword::FALSE},
    {"min", Keyword::MIN},
    {"max", Keyword::MAX},
    {"infinity", Keyword::INFINITY},
    {"nan", Keyword::NAN},
    {"epsilon", Keyword::EPSILON},
    {"this", Keyword::THIS},
    {"this-scope", Keyword::THIS_SCOPE},
    {"this-function", Keyword::THIS_FUNCTION},
    {"this-class", Keyword::THIS_CLASS},
    {"this-template", Keyword::THIS_TEMPLATE},
    {"this-concept", Keyword::THIS_CONCEPT},
    {"this-module", Keyword::THIS_MODULE},
    {"this-list", Keyword::THIS_LIST},
    {"this-file", Keyword::THIS_FILE},
    {"this-line", Keyword::THIS_LINE},
    {"this-column", Keyword::THIS_COLUMN},
    {"this-byte", Keyword::THIS_BYTE},
};

ostream &tonal::operator<<(ostream &out, const Token &token) {
  string indent(token.indent * 2, ' ');
  out << indent << token.detail.index() << ": ";
  visit(
      [&out, &token, &indent](const auto &detail) {
        using Detail = decay_t<decltype(detail)>;
        if constexpr (is_same_v<Detail, Token::List>) {
          if (!detail.end)
            out << "( " << (token.line + 1) << "," << (token.column + 1);
          else if (!detail.unpack)
            out << ") " << (token.line + 1) << "," << (token.column + 1);
          else
            out << ")... " << (token.line + 1) << "," << (token.column + 1);
        } else if constexpr (is_same_v<Detail, Token::Operator>) {
          out << detail.op;
        } else if constexpr (is_same_v<Detail, Token::Keyword>) {
          out << token.region << ": " << static_cast<int>(detail.keyword);
        } else if constexpr (is_same_v<Detail, Token::Identifier>) {
          if (detail.pack)
            cout << "...";
          for (auto &&q : detail.qualified)
            cout << q << " ";
          cout << detail.id;
          if (detail.unpack)
            cout << "...";
        } else if constexpr (is_same_v<Detail, Token::Number>) {
          cout << token.region << "\n"
               << indent << "   Sign: " << detail.sign << "\n"
               << indent << "   Base: " << detail.base << "\n"
               << indent << "   Numerator: " << detail.numerator << "\n"
               << indent << "   Decimal point: " << detail.decimal_point << "\n"
               << indent << "   Denominator: " << detail.denominator << "\n"
               << indent << "   Exponent point: " << detail.exponent_point
               << "\n"
               << indent << "   Exponent sign: " << detail.exponent_sign << "\n"
               << indent << "   Exponent: " << detail.exponent << "\n";
        } else if constexpr (is_same_v<Detail, Token::String>) {
          cout << "\n"
               << indent << "   Encoding: " << detail.encoding << "\n"
               << indent << "   Begin quote: " << detail.begin_quote << "\n"
               << indent << "   Begin delimiter: " << detail.begin_delimiter
               << "\n"
               << indent << "   Characters: "
               << (*cbegin(detail.begin_quote) == 'R'
                       ? regex_replace(string{detail.characters}, regex{"\n"},
                                       "\\n")
                       : detail.characters)
               << "\n"
               << indent << "   End delimiter: " << detail.end_delimiter << "\n"
               << indent << "   End quote: " << detail.end_quote << "\n";
        }
      },
      token.detail);
  return out;
}

Token::Token(string_view v) : region(v) {}

bool matches(string_view v, const regex &r, cmatch &m) {
  return regex_match(cbegin(v), cend(v), m, r,
                     regex_constants::match_continuous);
}

template <typename Exception>
void report_error(const string &message, size_t pos,
                  vector<Token>::const_iterator first,
                  vector<Token>::const_iterator current,
                  vector<Token>::const_iterator last) {
  static const auto line_pred = [](const Token &t) {
    return any_of(cbegin(t.region), cend(t.region),
                  [](char c) { return c == '\n'; });
  };
  auto head =
      max(find_if(reverse_iterator{current}, reverse_iterator{first}, line_pred)
                  .base() -
              1,
          first);
  auto line_end = max(find_if(current, last, line_pred) - 1, current);
  auto start = cbegin(head->region) + head->region.rfind('\n') + 1;
  string line = {start, cend(line_end->region)};
  string arrow(line.size(), ' ');
  auto token_start = distance(start, cbegin(current->region));
  auto token_end = max(token_start, distance(start, cend(current->region)) - 1);
  fill(begin(arrow) + token_start, begin(arrow) + token_end, '~');
  arrow[token_start] = arrow[token_end] = '+';
  pos += token_start;
  arrow[pos] = '^';
  string location = "Lexical error at line: " + to_string(current->line + 1) +
                    ", column: " + to_string(pos + 1) + '\n';
  equal(begin(arrow), end(arrow), cbegin(line), [](char &a, char l) {
    if (l == '\n')
      a = l;
    return true;
  });
  static const regex lf{"\n"};
  auto product = inner_product(
      sregex_token_iterator{cbegin(line), cend(line), lf, -1}, {},
      sregex_token_iterator{cbegin(arrow), cend(arrow), lf, -1}, string{},
      plus<>{}, [](const ssub_match &l, const ssub_match &r) {
        return l.str() + '\n' + (!r.length() ? "\n~\n" : r.str());
      });
  throw Exception{location + message + product};
}

static const regex pack_rx{R"(^\.\.\.(.*))"};
static const regex unpack_rx{R"((.*)\.\.\.$)"};
template <bool Pack = true, typename ReportLexicalError, typename TokenOffset>
Token::Identifier
validate_pack_unpack(cmatch match, ReportLexicalError &&report_lexical_error,
                     TokenOffset &&token_offset) {
  Token::Identifier t;
  t.id = string_view(match[1].first, match[1].length());
  t.pack = Pack;
  t.unpack = !Pack;

  auto offset = distance(match[0].first, match[1].first);
  string pack_unpack = Pack ? "pack" : "unpack";
  if (auto idx = t.id.find('.'); idx != string_view::npos)
    report_lexical_error("Period found in identifier " + pack_unpack + ":\n",
                         idx + offset);

  if (matches(t.id, regex{R"([^[:alpha:]_]+.*)"}, match))
    report_lexical_error("Identifier " + pack_unpack +
                             " must begin with letters or underscore:\n",
                         token_offset(match[0].first));

  if (identifier2keyword.find(t.id) != identifier2keyword.cend())
    report_lexical_error("Identifier " + pack_unpack +
                             " cannot be a keyword:\n",
                         token_offset(cbegin(t.id)));

  return t;
}

static const regex number_rx{R"((-|\+)?(0[[:alpha:]])?(\.?)(.*))"};
template <typename ReportLexicalError, typename TokenOffset>
Token::Number validate_number(cmatch match,
                              ReportLexicalError &&report_lexical_error,
                              TokenOffset &&token_offset) {
  Token::Number t;
  t.sign = string_view(match[1].first, match[1].length());
  t.base = string_view(match[2].first, match[2].length());
  t.decimal_point = string_view(match[3].first, match[3].length());
  t.numerator = string_view(match[4].first, match[4].length());

  int base = 10;
  char exponent_point = 'e';
  regex digits_rx{R"([^[:digit:]'_]+)"};
  if (t.base.length() > 1)
    switch (*(cbegin(t.base) + 1)) {
    default:
      report_lexical_error("Unknown base:\n", token_offset(cbegin(t.base)) + 1);
      break;
    case 'b':
      base = 2;
      digits_rx = R"([^01'_]+)";
      break;
    case 'o':
      base = 8;
      digits_rx = R"([^0-7'_]+)";
      break;
    case 'd':
      base = 10;
      digits_rx = R"([^[:digit:]'_]+)";
      break;
    case 'x':
      base = 16;
      exponent_point = 'p';
      digits_rx = R"([^[:xdigit:]'_]+)";
      break;
    case 'a':
      base = 36;
      exponent_point = '^';
      digits_rx = R"([^[:alnum:]'_]+)";
      break;
    case 's':
      base = 64;
      exponent_point = '^';
      digits_rx = R"([^[:alnum:]'_\\\+]+)";
      break;
    }

  const auto validate_digits = [&digits_rx, &report_lexical_error,
                                &token_offset](const string &aspect,
                                               const string_view &digits) {
    cmatch nondigit_match;
    if (regex_search(cbegin(digits), cend(digits), nondigit_match, digits_rx))
      report_lexical_error("Illegal character found in " + aspect + ":\n",
                           token_offset(nondigit_match[0].first));
  };

  const auto extract_exponent = [exponent_point, &report_lexical_error,
                                 &token_offset](string_view &v) {
    cmatch exponent_match;
    if (matches(v, regex{R"((.*?))"s + exponent_point + R"((-|\+)?(.*?))"},
                exponent_match)) {
      if (!exponent_match[3].length())
        report_lexical_error("Exponent not found:\n",
                             token_offset(exponent_match[1].second + 2));
      v = string_view(exponent_match[1].first, exponent_match[1].length());
      return make_tuple(
          string_view(exponent_match[1].second, 1),
          string_view(exponent_match[2].first, exponent_match[2].length()),
          string_view(exponent_match[3].first, exponent_match[3].length()));
    }
    return make_tuple(string_view(exponent_match[0].first, 0),
                      string_view(exponent_match[0].first, 0),
                      string_view(exponent_match[0].first, 0));
  };

  cmatch num;
  if (!matches(t.numerator, regex{R"(([^\.]*)(\.?)([^\.]*?))"}, num))
    report_lexical_error("Number token does not match:\n",
                         token_offset(cbegin(t.numerator)));
  if (!num[1].length() && !num[3].length())
    report_lexical_error("No digits found in number:\n",
                         token_offset(num[1].first));
  if (t.decimal_point.length() && num[2].length())
    report_lexical_error("Second decimal point found in number:\n",
                         token_offset(num[2].first));

  if (t.decimal_point.empty())
    t.decimal_point = string_view(num[2].first, num[2].length());

  if (num[2].length()) {
    t.numerator = string_view(num[1].first, num[1].length());
    if (auto pos = find(cbegin(t.numerator), cend(t.numerator), exponent_point);
        pos != num[1].second)
      report_lexical_error("Exponent point found before decimal point:\n",
                           token_offset(pos));

    t.denominator = string_view(num[3].first, num[3].length());
    tie(t.exponent_point, t.exponent_sign, t.exponent) =
        extract_exponent(t.denominator);
  } else {
    string_view numerominator(num[1].first, num[1].length());
    tie(t.exponent_point, t.exponent_sign, t.exponent) =
        extract_exponent(numerominator);
    (match[3].length() ? t.denominator : t.numerator) = numerominator;
    (match[3].length() ? t.numerator : t.denominator) =
        string_view{cbegin(numerominator), 0};
  }

  out << "Sign: " << t.sign << "\n";
  out << "Base: " << t.base << "\n";
  out << "Numerator: " << t.numerator << "\n";
  validate_digits("numerator", t.numerator);
  out << "Decimal point:" << t.decimal_point << "\n";
  out << "Denominator: " << t.denominator << "\n";
  validate_digits("denominator", t.denominator);
  out << "Exponent point: " << t.exponent_point << "\n";
  out << "Exponent sign: " << t.exponent_sign << "\n";
  out << "Exponent: " << t.exponent << "\n";
  validate_digits("exponent", t.exponent);

  return t;
}

static const regex string_rx{R"(((u(\d+))|R"|["'`])((?:.|[[:cntrl:]])*))"};
template <typename ReportLexicalError, typename TokenOffset>
Token::String validate_string(cmatch match,
                              ReportLexicalError &&report_lexical_error,
                              TokenOffset &&token_offset) {
  Token::String t;
  t.encoding = string_view(match[2].first, match[2].length());

  string_view str(match[0].first, match[0].length());
  int encoding = 8;
  if (match[3].matched) {
    if (match[3] == "8")
      encoding = 8;
    else if (match[3] == "16")
      encoding = 16;
    else if (match[3] == "32")
      encoding = 32;
    else
      report_lexical_error("Unrecognized literal string encoding:\n",
                           token_offset(match[3].first));
    str.remove_prefix(match[2].length());
  }

  t.begin_quote = string_view(cbegin(str), 2);
  t.end_quote = string_view(cend(str) - 1, 1);
  str.remove_suffix(1);
  switch (str.front()) {
  case 'R':
    str.remove_prefix(2);
    {
      auto delimiter = str.find('(') + 1;
      t.begin_delimiter = string_view(cbegin(str), delimiter - 1);
      str.remove_suffix(delimiter);
      str.remove_prefix(delimiter);
      t.characters = str;
      t.end_delimiter = string_view(cend(str) + 1, delimiter - 1);
      if (t.begin_delimiter != t.end_delimiter)
        report_lexical_error("Mismatching raw string delimiter:\n",
                             token_offset(cbegin(t.end_delimiter)));
    }
    break;
  case '"':
  case '\'':
  case '`':
    t.begin_quote.remove_suffix(1);
    str.remove_prefix(1);
    if (t.begin_quote != t.end_quote)
      report_lexical_error("Mismatching quote:\n",
                           token_offset(cbegin(t.end_quote)));
    out << "Quoted string: " << str << "\n";
    t.characters = str;
    static const regex escape_rx{
        R"(\\x[[:xdigit:]]+|\\[0-7]{1,3}|\\u.{0,4}|\\U.{0,8}|\\.)"};
    for_each(
        regex_iterator{cbegin(str), cend(str), escape_rx}, {},
        [&str, &report_lexical_error, &token_offset](cmatch match) {
          string_view special(match[0].first + 2, match[0].length() - 2);
          out << "Special: " << special << "\n";

          const auto validate_unicode = [&str, &report_lexical_error,
                                         &token_offset](const char *pos) {
            report_lexical_error(
                pos == cend(str)
                    ? "Insufficient characters found for unicode literal:\n"
                    : "Illegal character found in unicode literal:\n",
                token_offset(pos));
          };

          auto hexcount = 0;
          switch (match[0].first[1]) {
          case 'u':
            hexcount = 4;
            break;
          case 'U':
            hexcount = 8;
            break;
          }

          if (hexcount) {
            regex_search(cbegin(special), cend(special), match,
                         regex{R"([^[:xdigit:]])"});
            if (distance(cbegin(special), match[0].first) < hexcount)
              validate_unicode(match[0].first);
          }
        });
    break;
  }
  out << "Contents: " << str << "\n";

  return t;
}

template <typename ReportLexicalError, typename TokenOffset>
variant<Token::Operator, Token::Keyword, Token::Identifier>
validate_identifier(cmatch match, ReportLexicalError &&report_lexical_error,
                    TokenOffset &&token_offset) {
  string_view ident(match[0].first, match[0].length());
  if (matches(ident, regex{R"([[:punct:]]+)"}, match)) {
    if (regex_search(cbegin(ident), cend(ident), match, regex{R"(\.)"}))
      report_lexical_error("Period found in operator:\n",
                           token_offset(match[0].first));
    Token::Operator t;
    t.op = ident;
    return t;
  }

  if (identifier2keyword.find(ident) != identifier2keyword.cend()) {
    Token::Keyword t;
    t.keyword = identifier2keyword.at(ident);
    return t;
  }

  if (regex_search(cbegin(ident), cend(ident), match, regex{R"(\.\.+)"}))
    report_lexical_error("Empty segment in qualified identifier:\n",
                         token_offset(match[0].first) + 1);

  static const regex sep_rx{R"(\.)"};

  Token::Identifier t;
  transform(cregex_token_iterator{cbegin(ident), cend(ident), sep_rx, -1}, {},
            back_inserter(t.qualified), [](const csub_match &sub) {
              return string_view(sub.first, sub.length());
            });

  t.id = t.qualified.back();
  for (const auto &segment : t.qualified) {
    out << "Identifier segment: " << segment << "\n";
    cmatch match;
    if (!matches(segment, regex{R"([[:alpha:]_].*)"}, match))
      report_lexical_error("Identifier or identifier segment "
                           "must begin with a letter, underscore "
                           "or hyphen:\n",
                           token_offset(cbegin(segment)));
    if (identifier2keyword.find(segment) != identifier2keyword.cend())
      report_lexical_error("Identifier segment cannot be a keyword:\n",
                           token_offset(cbegin(segment)));
    if (matches(segment, regex{R"([[:punct:]]+)"}, match))
      report_lexical_error(
          "Operators not allowed as identifier or identifier segment:\n",
          token_offset(match[0].first));
  }
  t.qualified.pop_back();

  return t;
}

void validate(vector<Token>::iterator first, vector<Token>::iterator last) {
  for (auto next = first; next != last; ++next) {
    Token &t = *next;

    const auto report_lexical_error =
        [&first, &next, &last](const string &message, size_t pos) {
          report_error<invalid_argument>(message, pos, first, next, last);
        };

    const auto token_offset = [&t](const char *pos) {
      return distance(cbegin(t.region), pos);
    };

    cmatch match;
    if (t.region == "(") {
      out << "List begin\n";
      t.detail = Token::List{false, false};
    } else if (t.region == ")") {
      out << "List end\n";
      t.detail = Token::List{true, false};
    } else if (t.region == ")...") {
      out << "List unpack\n";
      t.detail = Token::List{true, true};
    } else if (matches(t.region, pack_rx, match)) {
      out << "Pack: " << match[1] << "\n";
      t.detail =
          validate_pack_unpack<true>(match, report_lexical_error, token_offset);
    } else if (matches(t.region, unpack_rx, match)) {
      out << "Unpack: " << match[1] << "\n";
      t.detail = validate_pack_unpack<false>(match, report_lexical_error,
                                             token_offset);
    } else if (matches(t.region, regex{R"([[:punct:][^\(\)\.]]+)"}, match)) {
      out << "Operator: " << t.region << "\n";
      auto ident =
          validate_identifier(match, report_lexical_error, token_offset);
      visit([&t](auto &&ident) { t.detail = ident; }, ident);
    } else if (matches(t.region,
                       regex{R"(((-|\+)|(0[[:alpha:]])|\.|([[:digit:]]+?)).*)"},
                       match)) {
      out << "Number: " << t.region << "\n";
      matches(t.region, number_rx, match);
      t.detail = validate_number(match, report_lexical_error, token_offset);
    } else if (matches(t.region, string_rx, match)) {
      out << "String: " << t.region << "\n";
      t.detail = validate_string(match, report_lexical_error, token_offset);
    } else if (matches(t.region, regex{R"([^[:space:]\(\)]+)"}, match)) {
      out << "Identifier: " << t.region << "\n";
      auto ident =
          validate_identifier(match, report_lexical_error, token_offset);
      visit([&t](auto &&ident) { t.detail = ident; }, ident);
    } else if (matches(t.region, regex{R"([[:space:]]+)"}, match)) {
      out << "Whitespace\n";
      t.detail = Token::Whitespace{};
    } else
      report_lexical_error("Unknown token:\n", 0);
    t.pos = distance(cbegin(first->region), cbegin(t.region));
  }
}

vector<Token> tonal::tokenize(string_view tokens) {
  static const regex token_rx{
      R"((?:u(?:\d+))?)" // String alternative encoding prefix
      /**/ R"((?:R"([^[:space:]\\\(\)]{0,16}?)\()" // Raw string delimiter
                                                   // specification
      /******/ R"((?:.|[[:cntrl:]])*?)"            //
      /****/ R"(\)\1")"              // Raw string delimiter termination
      /**/ R"(|"(?:\\.|[^\"\n])*?")" // String alternative - double quoted
      /**/ R"(|'(?:\\.|[^'\n])*?')"  // String alternative - single quoted
      /**/ R"(|`(?:\\.|[^`\n])*?`))" // String alternative - backtick quoted
      R"(|\()"                       // List alternative - start
      R"(|\)(?:\.\.\.)?)"            // List alternative - end, maybe unpack
      R"(|[^[:space:]\(\)]+)"        // Non-string, non-list, non-space
                                     // alternative
      R"(|[[:space:]]+)"             // Whitespace alternative
  };

  vector<Token> parsed;

  transform(regex_iterator{cbegin(tokens), cend(tokens), token_rx}, {},
            back_inserter(parsed), [](const cmatch &match) {
              return string_view(match[0].first, match[0].length());
            });

  for (auto &t : parsed)
    t.pos = distance(cbegin(tokens), cbegin(t.region));

  vector<int> token(parsed.size());
  iota(begin(token), end(token), 0);
  equal(begin(parsed), end(parsed), cbegin(token), [](Token &t, int token) {
    t.token = token;
    return true;
  });

  vector<int> line_count;
  transform(cbegin(parsed), cend(parsed), back_inserter(line_count),
            [](const Token &t) {
              return count(cbegin(t.region), cend(t.region), '\n');
            });
  partial_sum(cbegin(line_count), cend(line_count), begin(line_count));
  equal(begin(parsed), end(parsed), cbegin(line_count), [](Token &t, int line) {
    t.line = line;
    return true;
  });

  vector<int> column_count;
  transform(cbegin(parsed), cend(parsed), back_inserter(column_count),
            [tokens](const Token &t) {
              return distance(reverse_iterator{cbegin(t.region)},
                              find(reverse_iterator{cbegin(t.region)},
                                   reverse_iterator{cbegin(tokens)}, '\n'));
            });
  equal(begin(parsed), end(parsed), cbegin(column_count),
        [](Token &t, int column) {
          t.column = column;
          return true;
        });

  vector<int> paren_count;
  transform(cbegin(parsed), cend(parsed), back_inserter(paren_count),
            [](const Token &t) {
              if (t.region.empty())
                return 0;
              return t.region[0] == '(' ? 1 : 0;
            });
  partial_sum(cbegin(paren_count), cend(paren_count), begin(paren_count));
  equal(begin(parsed), end(parsed), cbegin(paren_count),
        [](Token &t, int paren) {
          t.paren = paren;
          return true;
        });

  vector<int> close_count;
  transform(cbegin(parsed), cend(parsed), back_inserter(close_count),
            [](const Token &t) {
              if (t.region.empty())
                return 0;
              return t.region[0] == ')' ? -1 : 0;
            });
  partial_sum(cbegin(close_count), cend(close_count), begin(close_count));
  equal(
      begin(parsed), end(parsed), cbegin(close_count), [](Token &t, int close) {
        t.indent = t.paren + close - (!t.region.empty() && t.region[0] == '(');
        return true;
      });

  validate(begin(parsed), end(parsed));

  return parsed;
}
