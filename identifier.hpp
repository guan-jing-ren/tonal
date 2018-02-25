#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace tonal {
using namespace std;

struct invalid_identifier : invalid_argument {
  using invalid_argument::invalid_argument;
};

struct Identifier {
  Identifier(const string_view &);
  Identifier(const Identifier &);
  Identifier(Identifier &&) = default;
  Identifier &operator=(const Identifier &);
  Identifier &operator=(Identifier &&) = default;

  string_view scope() const;
  string_view name() const;

  // type
  // scope
  // visibility
  // module - owning module
  // class - owning class
  // function - owning function
  // template
  // visibility
  // file
  // line
  // column
  // absolute - number of bytes encountered
  // relative - number of parentheses encountered

private:
  string_view m_fully_qualified;
  string_view m_scope;
  string_view m_name;

  friend class std::hash<Identifier>;
};
} // namespace tonal

template <> struct ::std::hash<tonal::Identifier> {
  uint32_t operator()(const tonal::Identifier &) const;
};
