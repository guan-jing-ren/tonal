#pragma once

#include "symboltable.hpp"

#include "entity.hpp"
#include "identifier.hpp"

#include <memory>
#include <unordered_map>

namespace tonal {

struct SymbolTable {
private:
  unordered_multimap<Identifier, Entity> m_symbols;
};

} // namespace tonal
