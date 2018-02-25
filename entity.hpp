#pragma once

#include <typeindex>

namespace tonal {
using namespace std;

extern const type_index TypeList;
extern const type_index TypeVariable;
extern const type_index TypeFunction;
extern const type_index TypeClass;
extern const type_index TypeConcept;
extern const type_index TypeModule;

struct Entity {
  type_index type();

private:
};
} // namespace tonal
