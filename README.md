![Test by GCC latest](https://github.com/onihusube/harmony/workflows/Test%20by%20GCC%20latest/badge.svg)

# harmony

"harmony" is a header only library for working with monad in the C++ world.

It identifies monadic types by CPO and concept, and adds support for *bind* and some monadic operations.

A monadic type, for example...

- Pointer
- Smart Pointer (`std::unique_ptr<T>, std::shared_ptr<T>`)
- `std::optional<T>`
- Containers (`std::vector<T>, std::list<T>... etc`)
- `Either<L, R>` (`Result<T, E>`) like types
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
- All *bind* operator (`operator|`) is use *Hidden friends* idiom
- Header only
- Requires C++20 or later
    - GCC 10.1 or later
    - MSVC 2019 latest
      - Some tests have been disabled

## Facility

### concept `uwrappable`

The `uwrappable` concept determines whether a type is monadic and is a fundamental concept in this library.

It's defined as follows:

```cpp
template<typename T>
concept unwrappable = requires(T&& m) {
  { harmony::cpo::unwrap(std::forward<T>(m)) } -> not_void;
};
```

It is required to be able to retrieve the value contained in the type by `unwrap` CPO.

#### CPO `unwrap`


### concept `maybe` `list`

#### CPO `validate`

### concept `rewrappable`

#### CPO `unit`

### concept `monadic`

### concept `either`

#### CPO `unwrap_other`

### monadic operation `map/transform`
### monadic operation `map_err`
### monadic operation `and_then`
### monadic operation `or_else`
### monadic operation `match/fold`
### monadic operation `exists`
### monadic operation `try_catch`
### type `monas<T>`
### type `sachet<L, R>`
### operation `to_value<T>`

