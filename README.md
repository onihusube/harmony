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
    - MSVC 2019 Preview latest

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

The name `harmony::unwrap` denotes a customization point object.

Given a subexpression `E` with type `T`, let `t` be an lvalue that denotes the reified object for `E`. Then:

1. If `T` is an pointer type or indirectly readable (by `operator*`) class type, `harmony::unwrap(E)` is expression-equivalent to `*t`.
2. Otherwise, if `t.value()` is a valid expression whose type not void, `harmony::unwrap(E)` is expression-equivalent to `t.value()`.
3. Otherwise, if `t.unwrap()` is a valid expression whose type not void, `harmony::unwrap(E)` is expression-equivalent to `t.unwrap()`.
4. Otherwise, if `T` modeles `std::ranges::range`, `harmony::unwrap(E)` is `E`.
5. Otherwise, `harmony::unwrap(E)` is ill-formed.

If `E` is an rvalue, we get the same result as above with `t` as the rvalue.

### concept `maybe` `list`

The types that modeled `maybe`, `list` correspond to *maybe monad*, *list monad*, respectively.

```cpp
template<typename T>
concept maybe =
 unwrappable<T> and
 requires(const T& m) {
   { harmony::cpo::validate(m) } -> std::same_as<bool>;
 };

template<typename T>
concept list = maybe<T> and std::ranges::range<T>;
```

`list` is `maybe` and `range`, `maybe` is `uwrappable` and requires that it is possible to determine if the contents are present by `validate` CPO.

#### CPO `validate`

The name `harmony::validate` denotes a customization point object.

Given a subexpression `E` with type `T`, let `t` be an const lvalue that denotes the reified object for `E`. Then:

1. If `T` not modeles `unwrappable`, `harmony::validate(E)` is ill-formed.
2. If `bool(t)` is a valid expression, `harmony::validate(E)` is expression-equivalent to `bool(t)`.
3. Otherwise, if `t.has_value()` is a valid expression, `harmony::validate(E)` is expression-equivalent to `t.has_value()`.
4. Otherwise, if `t.is_ok()` is a valid expression, `harmony::validate(E)` is expression-equivalent to `t.is_ok()`.
5. Otherwise, if `std::ranges::empty(t)` is a valid expression, `harmony::validate(E)` is expression-equivalent to `std::ranges::empty(t)`.
6. Otherwise, `harmony::validate(E)` is ill-formed.

Whenever  `harmony::validate(E)` is a valid expression, it has type bool. 

### concept `rewrappable`

`rewrappable` indicates that the value of type `T` can be *unit* (or *return*) for an object of type `M`.

```cpp
template<typename M, typename T>
concept rewrappable = 
  unwrappable<M> and
  requires(M& m, T&& v) {
    harmony::cpo::unit(m, std::forward<T>(v));
  };
```

This is also defined by `unit` CPO.

#### CPO `unit`

The name `harmony::unit` denotes a customization point object.

Given a subexpression `E` and `F` with type `T` and `U`, let `t, u` be an lvalue that denotes the reified object for `E, F`, let `m` that denotes the result for `cpo::unwrap(t)`. Then:

1. If `T` not modeles `unwrappable`, `harmony::unit(E, F)` is ill-formed.
2. Otherwise, If `m` is lvalue reference, `decltype((m))` and `U` models `std::assignable_from`, `harmony::unit(E, F)` is expression-equivalent to `m = u`.
3. Otherwise, if `T&` and `U` models `std::assignable_from`, `harmony::unit(E, F)` is expression-equivalent to `t = u`.
4. Otherwise, `harmony::unit(E, F)` is ill-formed.

If `F` is an rvalue, we get the same result as above with `u` as the rvalue.

### concept `monadic`

`monadic` indicates that the result of applying *callable* `F` to the contents of `unwrappable M` can be reassigned by `unit` CPO.

```cpp
template<typename F, typename M>
concept monadic = 
  std::invocable<F, traits::unwrap_t<M>> and
  rewrappable<M, std::invoke_result_t<F, traits::unwrap_t<M>>>;
```

### concept `either`

The type that models `either` corresponds to *Either monad*.

```cpp
template<typename T>
concept either = 
  maybe<T> and
  requires(T&& t) {
    {cpo::unwrap_other(std::forward<T>(t))} -> not_void;
  };
```

`either` is `maybe`, and indicates that an invalid value (equivalent to) can be retrieved.

This is also defined by `unwrap_other` CPO.

#### CPO `unwrap_other`

The name `harmony::unwrap_other` denotes a customization point object.

Given a subexpression `E` with type `T`, let `t` be an lvalue that denotes the reified object for `E`. Then:

1. If `T` not modeles `maybe`, `harmony::unwrap_other(E)` is ill-formed.
2. If `T` is an specialization of `std::optional`, `harmony::unwrap_other(E)` is `std::nullopt`.
3. Otherwise, if `T` is an pointer type or pointer like type (e.g smart pointer types), `harmony::unwrap_other(E)` is `nullptr`.
4. Otherwise, if `t.error()` is a valid expression whose type not void, `harmony::unwrap_other(E)` is expression-equivalent to `t.error()`.
5. Otherwise, if `t.unwrap_err()` is a valid expression whose type not void, `harmony::unwrap_other(E)` is expression-equivalent to `t.unwrap_err()`.
6. Otherwise, `harmony::unwrap_other(E)` is ill-formed.

If `E` is an rvalue, we get the same result as above with `t` as the rvalue.

### type `monas<T>`

`harmony::monas` is the starting point for using the facilities of this library. It's a thin wrapper for monadic types.

### operator *bind*

This library uses `operator|` as the bind operator (e.g `>>=`).

```cpp
#include <iostream>
#include <optional>

// Main header of this library
#include "harmony.hpp"

int main() {
  
  std::optional<int> opt = 10;

  // Process chaining
  std::optional<int> result = harmony::monas(opt) | [](int n) { return n + n; }
                                                  | [](int n) { return n + 100;};
  
  std::cout << *result; // 120
}
```
[[Wandbox]三へ( へ՞ਊ ՞)へ ﾊｯﾊｯ](https://wandbox.org/permlink/uNelzmnVHY8rXGJk)

(The code at the beginning is republished.)

You can chain any number of operations on valid values. They will not be called on invalid values.

However, if you want to change the type, use `map`.

### monadic operation `map(transform)/map_err`

`map/transform` performs the conversion of valid values and `map_err` performs the conversion of invalid values. 

`transform` is a mere alias for `map`.

```cpp
int main() {
  using namespace harmony::monadic_op;

  // Conversion of valid value. int -> double
  auto result = std::optional<int>{10} | map([](int n) { return double(n) + 0.1; });
  // decltype(result) is not std::optional<double>, but a type like Either<double, nullopt_t>.
  
  std::cout << harmony::unwrap(result) << std::endl; // 10.1

  // Conversion of invalid value. std::nullopt_t -> bool
  auto err = std::optional<int>{} | map_err([](std::nullopt_t) { return false; });
  // decltype(err) is not std::optional<bool>, but a type like Either<int, bool>.

  std::cout << std::boolalpha << harmony::unwrap_other(err);  // false
}
```
[[Wandbox]三へ( へ՞ਊ ՞)へ ﾊｯﾊｯ](https://wandbox.org/permlink/ECRybBd3uTEAiaYd)

Both take one Callable object `f`, as an argument. The return type of `f` is arbitrary, but the result is wrapped in `harmony::monas` (So you can continue to chain *bind* and other monadic operations.).

The type on the left side of `| map(...)` must models `either`.

### monadic operation `and_then/or_else`

`and_then` and `or_else` are similar to `map` and `map_err`. The difference is that the Callable return type that you receive must be modeles `either`.

The return type in both cases must be able to accept the other unconverted value as is.

```cpp
int main() {
  using namespace harmony::monadic_op;

  // Conversion of valid value. int -> double
  auto andthen = std::optional<int>{10} | and_then([](int n) { return std::optional<double>(double(n) + 0.1); });
  // decltype(*andthen) is std::optional<double>.
  
  std::cout << harmony::unwrap(andthen) << std::endl; // 10.1

  // Conversion of invalid value. std::nullopt_t -> bool
  auto orelse = std::optional<int>{} | or_else([](std::nullopt_t) { return std::optional<double>(-0.0); });
  // decltype(*orelse) is std::optional<double>.

  std::cout << harmony::unwrap(orelse);  // -0.0
}
```
[[Wandbox]三へ( へ՞ਊ ՞)へ ﾊｯﾊｯ](https://wandbox.org/permlink/iMGfQS9tVB2jYFgl)

These also wrap `either` return type with `harmony::monas` (So you can continue to chain *bind* and other monadic operations.).

The type on the left side of `| and_then(...)`(`| or_else(...)`) must models `either`.

### monadic operation `match(fold)`

`match` takes a process for each valid and invalid value and applies it appropriately, depending on the state of the object.

However, the return type must be aggregated into one type.

`fold` is a mere alias for `match`.

```cpp
int main() {
  using namespace harmony::monadic_op;

  int n = 10;

  int r = harmony::monas(&n)
    | match([](int n){ return 2*n;},          // Processing for valid values
            [](std::nullptr_t) { return 0;}); // Processing for invalid values

  std::cout << r << std::endl;  // 20

  int *p = nullptr;

  r = harmony::monas(p)
    | match([](int){ return 0;}, [](std::nullptr_t) { return 1;});

  std::cout << r << std::endl;  // 1
}
```
[[Wandbox]三へ( へ՞ਊ ՞)へ ﾊｯﾊｯ](https://wandbox.org/permlink/xe3iSYYj3HqzkZb2)

You can also pass only one Callable object to `match` (e.g generic lambda).

The type on the left side of `| match(...)` must models `either`.

### monadic operation `exists`

`exists` applies the predicate and returns the result if the target object has a valid value. If it has an invalid value, it immediately returns false.

```cpp
int n = 10;

bool r = harmony::monas(&n)
  | [](int n) { return n + n; }
  | exists([](int n) { return n == 20;});
// r == true
```

It also behaves like `list` for `std::any_of`.

```cpp
std::vector<int> vec = {2, 4, 6, 8, 10};

bool r = vec | exists([](int n) { return n == 8; });
// r == true
```

The type on the left side of `| exists(...)` must models `maybe`.

### monadic operation `try_catch`

`try_catch` takes a callable `f` and its arguments and returns Either with its result and the `std::exception_ptr`.

```cpp
int main() {
  using namespace harmony::monadic_op;

  // Processing that can throw an exception
  auto f = [](int n, int m) -> int {
     if (m == 0) throw "division by zero";
     return n / m;
   };

  auto r = try_catch(f, 4, 2)
    | map([](int n) { return n == 2; });

  std::cout << std::boolalpha << harmony::unwrap(r) << std::endl; // true

  auto str = try_catch(f, 4, 0)
    | map_err([](std::exception_ptr exptr) { 
        try { std::rethrow_exception(exptr); }
        catch(const char* message) {
          return std::string{message};
        }
      });

  std::cout << harmony::unwrap_other(str);  // division by zero
}
```
[[Wandbox]三へ( へ՞ਊ ՞)へ ﾊｯﾊｯ](https://wandbox.org/permlink/4mAQJNX5pdYv2Qsb)


### operation `map_to<T>/fold_to<T>`

`map_to<T>` and `fold_to<T>` are `map` and `fold(match)` convenience operations, respectively. They return the converted value directly.

Using this, the previous code can be written as follows

```cpp
int main() {
  using namespace harmony::monadic_op;

  // Processing that can throw an exception
  auto f = [](int n, int m) -> int {
     if (m == 0) throw "division by zero";
     return n / m;
   };

  bool r = try_catch(f, 4, 2)
    | map([](int n) { return n == 2; })
    | map_to<bool>;

  std::cout << std::boolalpha << r << std::endl; // true

  std::string str = try_catch(f, 4, 0)
    | map([](int) { return std::string{}; })  // To match the type
    | map_err([](std::exception_ptr exptr) { 
        try { std::rethrow_exception(exptr); }
        catch(const char* message) {
          return std::string{message};
        }
      })
    | fold_to<std::string>;

  std::cout << str;  // division by zero
}
```
[[Wandbox]三へ( へ՞ਊ ՞)へ ﾊｯﾊｯ](https://wandbox.org/permlink/cOv5nwH5CupsQHDE)

If a type that is merely a `maybe` has an invalid value, it returns the default constructed value (if possible).

Also, in both cases, narrowing conversion is not allowed.
