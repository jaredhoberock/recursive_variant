// clang -std=c++17 demo.cpp -lstdc++
#include "recursive_variant.hpp"

struct forward_declared;

// forward_declared not yet complete when recursive_variant is instantiated
using my_variant = recursive_variant<forward_declared>;

// OK, forward_declared is complete now
struct forward_declared {};

int main()
{
  my_variant var;

  return 0;
}

