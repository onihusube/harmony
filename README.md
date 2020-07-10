# harmony

"harmony" is a header only library for working with monad in the C++ world.

It identifies monadic types by CPO and concept, and adds support for *bind* and some monadic operations.

A monadic type, for example...

- Pointer
- Smart Pointer (`std::unique_ptr<T>, std::shared_ptr<T>`)
- `std::optional<T>`
- Container types (`std::vector<T>, std::list<T>... etc`)
    - Under implementation
- Any program defined types that can recognized monad

## Example

```cpp
#include <iostream>
#include <optional>

// Main header of this library
#include "harmony.hpp"

int main() {
  
  std::optional<int> opt = 10;

  // Processing chaining
  std::optional<int> result = harmony::monas(opt) | [](int n) { return n + n; }
                                                  | [](int n) { return n + 100;};
  
  std::cout << *result; // 120
}
```
[[Wandbox]三へ( へ՞ਊ ՞)へ ﾊｯﾊｯ](https://wandbox.org/permlink/uNelzmnVHY8rXGJk)

```cpp
#include <iostream>
#include <optional>

#include "harmony.hpp"

int main() {
  
  std::optional<int> opt = 10;
  
  std::optional<int> result = harmony::monas(opt) | [](int n) { return n + n; }
                                                  | [](int n) { return n + 100; }
                                                  | [](int)   { return std::nullopt; }  // A processsing that fails
                                                  | [](int n) { return n*n; };
  
  if (harmony::validate(result)) {
    std::cout << *result;
  } else {
    std::cout << "failed!"; // This is called
  }
}
```
[[Wandbox]三へ( へ՞ਊ ՞)へ ﾊｯﾊｯ](https://wandbox.org/permlink/uNelzmnVHY8rXGJk)

## Overview

- Generic library based on Customization Point Object (CPO) and Concept
- Header only
- Requires C++20 or later
    - Currently only supported by GCC 10.1