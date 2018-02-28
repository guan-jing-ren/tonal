#include "tonal.hpp"
#include "token.hpp"

#include <algorithm>
#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

using namespace tonal;
using namespace experimental;

string read_file(ifstream stream) {
  stringbuf source;
  while (stream) {
    stream.get(source);
    source.sputc('\n');
    stream.clear();
    stream.get(); // .get does not extract \n
  }
  auto str = source.str();
  return str;
}

template <typename T> vector<shared_ptr<T>> memory_sorted(size_t n) {
  vector<shared_ptr<T>> sorted;
  sorted.reserve(n);
  generate_n(back_inserter(sorted), n, []() { return make_shared<T>(); });
  sort(begin(sorted), end(sorted));
  return sorted;
}

template <typename T> vector<shared_ptr<T>> memory_sorted(vector<T> &&v) {
  auto sorted = memory_sorted<T>(v.size());
  equal(begin(sorted), end(sorted), cbegin(v), [](auto &shared, auto &object) {
    *shared = move(object);
    return true;
  });
  return sorted;
}

class List;

class Value {};

class Parameter;

class Id {
public:
  shared_ptr<const Token> token;

  class Hash {
  public:
    uint32_t operator()(const Id &id) const {
      return hash<decltype(token)>{}(id.token);
    }
  };
};

class Variable {
public:
  shared_ptr<const Token> location;
  vector<shared_ptr<Parameter>> parameters;
};

class ReturnType {};

class Scope {
  shared_ptr<const Token> location;
};

class Function {
public:
  shared_ptr<const Token> location;
  vector<shared_ptr<Variable>> captures;
  vector<shared_ptr<Parameter>> parameters;
  shared_ptr<ReturnType> return_type;
  vector<shared_ptr<List>> body;
};

class Concept {
public:
  shared_ptr<const Token> location;
  vector<shared_ptr<Parameter>> parameters;
  vector<shared_ptr<Concept>> bases;
  vector<shared_ptr<Function>> functions;
};

class Class {
public:
  shared_ptr<const Token> location;
  vector<shared_ptr<Parameter>> parameters;
  vector<shared_ptr<Concept>> bases;
  vector<shared_ptr<Variable>> data;
  vector<shared_ptr<Function>> functions;
};

class Parameter {
public:
  shared_ptr<const Token> location;
  variant<shared_ptr<Concept>, shared_ptr<Class>, shared_ptr<Value>> type;
};

class Module {
public:
  unordered_map<shared_ptr<const Id>, vector<shared_ptr<const Concept>>>
      concepts;
  unordered_map<shared_ptr<const Id>, vector<shared_ptr<const Class>>> classes;
  unordered_map<shared_ptr<const Id>, vector<shared_ptr<const Function>>>
      functions;
  unordered_map<shared_ptr<const Id>, vector<shared_ptr<const Variable>>>
      variables;
};

// class Entity {
// public:
//   vector<Entity> resolve_stack;
// };

class List : public Entity {
public:
  List() = default;
  List(const shared_ptr<Token> &h) { head = h; }

  shared_ptr<const Token> head;
  shared_ptr<const Token> tail;
};

class ParseState {
  class LexicalScope {
  public:
    Concept &getConcept() { return *get<0>(scope); }
    Class &getClass() { return *get<1>(scope); }
    Function &getFunction() { return *get<2>(scope); }
    Scope &getScope() { return *get<3>(scope); }

    template <typename T>
    LexicalScope(T &&t) : scope(make_shared<decay_t<T>>(forward<T>(t))) {}

  private:
    variant<shared_ptr<Concept>, shared_ptr<Class>, shared_ptr<Function>,
            shared_ptr<Scope>>
        scope;
  };

  class ListIterator {
  public:
    bool at_head() const { return *iter == list->head; }
    bool at_tail() const { return *iter == list->tail; }

    const shared_ptr<const Token> &operator*() { return *iter; }
    const Token *operator->() { return iter->get(); }

    ListIterator &operator++() {
      while ((*iter)->indent > list->tail->indent + 1)
        ++iter;
      if (*iter != list->tail)
        ++iter;
      return *this;
    }

    ListIterator &operator--() {
      while ((*iter)->indent > list->head->indent + 1)
        --iter;
      if (*iter != list->head)
        --iter;
      return *this;
    }

    shared_ptr<const List> list;
    vector<shared_ptr<const Token>>::const_iterator iter;
  };

public:
  static map<filesystem::path, shared_ptr<const string>> sources;
  static map<filesystem::path, shared_ptr<ParseState>> states;
  static vector<shared_ptr<Module>> modules;

  const vector<shared_ptr<const Token>> tokens;
  const vector<shared_ptr<const List>> lists;

  shared_ptr<Module> current_module;
  deque<LexicalScope> current_scope;

  vector<shared_ptr<const List>> current_list;
  vector<shared_ptr<const Token>> current_token;

  ParseState(vector<shared_ptr<Token>> &&t, vector<shared_ptr<List>> &&l)
      : tokens(begin(t), end(t)), lists(begin(l), end(l)) {
    current_module = make_shared<Module>();
    modules.push_back(current_module);
  }

  static void compile(const filesystem::path &path) {
    const auto full_path = canonical(absolute(path));
    sources[full_path] =
        make_shared<string>(read_file(ifstream{full_path.u8string()}));

    auto file_tokens = tokenize(*sources[full_path]);
    for (auto &t : file_tokens)
      if (t.detail.index() != static_cast<char>(TokenType::WHITESPACE))
        cout << t << "\n";

    auto tokens = memory_sorted(move(file_tokens));

    vector<List> list_tokens;
    copy_if(
        cbegin(tokens), cend(tokens), back_inserter(list_tokens),
        [](auto &&token) {
          bool copy = false;
          visit(
              [&copy](auto &&detail) {
                if constexpr (is_same_v<Token::List, decay_t<decltype(detail)>>)
                  copy = !detail.end && !detail.unpack;
              },
              token->detail);
          return copy;
        });

    auto lists = memory_sorted(move(list_tokens));
    for (auto &&list : lists)
      list->tail =
          *find_if(lower_bound(cbegin(tokens), cend(tokens), list->head),
                   cend(tokens), [&list](auto &&token) {
                     bool found = false;
                     visit(
                         [&found](auto &&detail) {
                           if constexpr (is_same_v<Token::List,
                                                   decay_t<decltype(detail)>>)
                             found = detail.end;
                         },
                         token->detail);
                     return found && list->head->indent == token->indent;
                   });

    (states[full_path] = make_shared<ParseState>(move(tokens), move(lists)))
        ->parse_file();
  }

  const shared_ptr<const List> &
  find_list(const shared_ptr<const Token> &token) const {
    return *lower_bound(cbegin(lists), cend(lists), token,
                        [](auto &&l, auto &&r) {
                          using L = decay_t<decltype(l)>;
                          using R = decay_t<decltype(r)>;

                          if constexpr (is_same_v<L, List>)
                            return l.head < r;
                          else if constexpr (is_same_v<R, List>)
                            return l < r.head;
                          static_assert(!is_same_v<L, R>);
                          return false;
                        });
  }

  ListIterator iterate_list(const shared_ptr<const List> &list) {
    ListIterator iter;
    iter.list = list;
    iter.iter = ++lower_bound(cbegin(tokens), cend(tokens), list->head);
    return iter;
  }

  // vector<shared_ptr<const List>>
  // list_path(const shared_ptr<const Token> &token) {
  //   vector<shared_ptr<const List>> path;
  //   path.push_back(find_list(token));
  //   find_if(
  //       reverse_iterator(lower_bound(cbegin(lists), cend(lists),
  //       path.back())), reverse_iterator(cbegin(lists)), [&path](const auto
  //       &list) {
  //         return list->head->indent < path.back()->head->indent;
  //       });
  //   return path;
  // }

  void parse_file() {
    for (auto &&list : lists)
      if (list->head->indent == 0)
        process_list(list);
  }

  void process_list(const shared_ptr<const List> &list) {
    // cout << *list->head << " ... ... ... " << *list->tail << "\n";

    current_list.push_back(list);
    auto iter = iterate_list(list);
    visit(
        [this /*, &token */](auto &&detail) {
          using Detail = decay_t<decltype(detail)>;
          if constexpr (is_same_v<Detail, Token::List>) {
            // process_list(find_list(token));
          } else if constexpr (is_same_v<Detail, Token::Identifier>) {
          } else if constexpr (is_same_v<Detail, Token::Keyword>) {
            switch (detail.keyword) {
            case Keyword::MODULE:
              declare_module();
              break;
            case Keyword::CONCEPT:
              declare_concept();
              break;
            case Keyword::CLASS:
              declare_class();
              break;
            case Keyword::FUNCTION:
              declare_function();
              break;
            }
          }
        },
        iter->detail);

    current_list.pop_back();
  }

  // Parse for identifier names first, skip descriptions
  // Parse identifier names as far as parameters
  // - parameters for identifiers for disambiguation
  // Identifiers are visible once parsed and checked for duplicates

  void declare_module() {
    // 1) Keyword
    // 2) Identifier

    current_module = make_shared<Module>();
    shared_ptr<const Id> id;

    auto iter = iterate_list(current_list.back());
    ++iter;
    visit(
        [](auto &&detail) {
          using Detail = decay_t<decltype(detail)>;
          if constexpr (is_same_v<Detail, Token::Identifier>) {
            cout << "MODULE: " << detail.id << "\n";
          }
        },
        iter->detail);
    if (iter.at_tail() || !(++iter).at_tail())
      ;

    modules.push_back(current_module);
  }
  void declare_concept() {
    // 1) Keyword
    // 2) Identifier
    // 3) Concept or literal parameters
    // 4) Inherited concepts (: ...)
    // 5) Members and functions
    LexicalScope &scope = current_scope.back();
    current_scope.push_back(Concept{});
    shared_ptr<const Id> id; // Append current concept or class
    current_scope.back().getConcept().location = current_list.back()->head;
    current_scope.pop_back();
  }
  void declare_class() {
    // 1) Keyword
    // 2) Identifier
    // 3) Concept or literal parameters
    // 4) Inherited classes (: ...)
    // 5) Members and functions
    current_scope.push_back(Class{});
    shared_ptr<const Id> id; // Append current concept or class.
    current_scope.pop_back();
  }
  void declare_function() {
    // 1) Keyword
    // 2) Identifier
    // 3) Capture list (: ...)
    // 4) Concept or literal or variable parameters
    // 5) Concept or literal or variable Return type
    // 6) Function body in description
    current_scope.push_back(Function{});
    shared_ptr<const Id> id; // Append current concept or class.
    current_scope.pop_back();
  }
  void declare_scope() {
    current_scope.push_back(Scope{});
    current_scope.pop_back();
  }
  void declare_variable() {
    // 1) Resolved type
    // 2) Identifier
  }
  void declare_identifier() {
    // - Check for conflicts
  }
  void declare_parameter() {}
  void declare_alias() {
    // - Using free function as member function
    // - Using inherited function as member function
    // - Using module namespace
    // - Using module entities
    // - Using source declaration
    // - Reverse the effects of using

    // Build hierarchy of search order
  }
  void declare_label() {}
};

map<filesystem::path, shared_ptr<const string>> ParseState::sources;
map<filesystem::path, shared_ptr<ParseState>> ParseState::states;
vector<shared_ptr<Module>> ParseState::modules;

// Parameter matching - packs and concepts
// Concept matching

int main(int, char **v) {
  try {
    ParseState::compile(v[1]);
  } catch (invalid_argument e) {
    cout << e.what() << "\n";
  }

  return 0;
}
