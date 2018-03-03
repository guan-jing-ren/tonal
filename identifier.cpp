#include "identifier.hpp"

#include <regex>

using namespace tonal;

static const regex identifier_rx{
    "[_\\-[:alpha:]][_\\-.[:alnum:]]*[_\\-[:alnum:]]+"};

Identifier::Identifier(const string_view &ident)
    : m_fully_qualified(ident), m_scope(m_fully_qualified),
      m_name(m_fully_qualified) {
  if (!regex_match(begin(m_fully_qualified), end(m_fully_qualified),
                   identifier_rx))
    throw invalid_identifier{"Invalid identifier: " +
                             string{m_fully_qualified}};

  m_scope.remove_suffix(m_scope.size() - m_scope.rfind('.'));
  m_name.remove_prefix(m_name.rfind('.') + 1);
}

string_view Identifier::name() const { return m_name; }

string_view Identifier::scope() const { return m_scope; }

uint32_t std::hash<Identifier>::operator()(const Identifier &ident) const {
  return hash<string_view>{}(ident.m_fully_qualified);
}
