#include <optional>
#include <vector>
#include <cmath>
#include <string>

#include "boost/ut.hpp"
#include "harmony.hpp"

template<typename T, typename E>
struct simple_result {
  T ok;
  E err;
  bool is_ok_v = true;

  template<typename U = T>
    requires std::constructible_from<T, U&&>
  simple_result(U&& v)
    : ok(std::forward<U>(v))
    , err{}
    , is_ok_v{true}
  {}

  template<typename U = E>
    requires std::constructible_from<E, U&&>
  simple_result(E&& v)
    : ok{}
    , err(std::forward<U>(v))
    , is_ok_v{true}
  {}

  auto unwrap() -> T& {
    return ok;
  }

  auto error() -> E& {
    return err;
  }

  bool is_ok() const noexcept {
    return is_ok_v;
  }
};


namespace ut = boost::ut;

int main() {
  using namespace boost::ut::literals;
  using namespace boost::ut::operators::terse;

  harmony::sachet<double> t{1.0};
  harmony::unit(t, 2.0);

  "concept unwrappable test"_test = [] {
    ut::expect(harmony::unwrappable<int*>);
    ut::expect(harmony::unwrappable<std::optional<int>>);
    ut::expect(harmony::unwrappable<std::vector<int>>);
    ut::expect(harmony::unwrappable<simple_result<int, std::string>>);
  };

  "concept maybe test"_test = [] {
    ut::expect(harmony::maybe<int *>);
    ut::expect(harmony::maybe<std::optional<int>>);
    ut::expect(harmony::maybe<std::vector<int>>);
    ut::expect(harmony::maybe<simple_result<int, std::string>>);
  };

  "concept list test"_test = [] {
    ut::expect(not harmony::list<int *>);
    ut::expect(not harmony::list<std::optional<int>>);
    ut::expect(harmony::list<std::vector<int>>);
  };

  "concept rewrappable test"_test = [] {
    ut::expect(harmony::rewrappable<int*, int>);
    ut::expect(harmony::rewrappable<std::optional<int>, int>);
    ut::expect(harmony::rewrappable<int*, short>);
    ut::expect(harmony::rewrappable<std::optional<int>, short>);
    ut::expect(harmony::rewrappable<int*, double>);
    ut::expect(harmony::rewrappable<std::optional<int>, double>);
    ut::expect(harmony::rewrappable<std::optional<int>, std::optional<int>>);
    ut::expect(harmony::rewrappable<int*, int*>);
    ut::expect(harmony::rewrappable<simple_result<int, std::string>, int>);
    ut::expect(not harmony::rewrappable<simple_result<int, std::string>, std::string>);
  };

  "concept either test"_test = [] {
    ut::expect(harmony::either<int*>);
    ut::expect(harmony::either<std::optional<int>>);
    ut::expect(harmony::either<simple_result<int, std::string>>);
  };

  "cpo unwrap test"_test = [] {
    {
      int n = 10;
      int *p = &n;

      10_i == harmony::unwrap(p);

      n = 20;

      20_i == harmony::unwrap(p);
    }
    {
      std::optional<int> opt = 10;

      10_i == harmony::unwrap(opt);

      opt = 20;

      20_i == harmony::unwrap(opt);
    }
  };

  "cpo validate test"_test = [] {
    {
      int *p = nullptr;

      ut::expect(not harmony::validate(p));

      int n = 10;
      p = &n;

      ut::expect(harmony::validate(p));
    }
    {
      std::optional<int> opt = std::nullopt;

      ut::expect(not harmony::validate(opt));

      opt = 10;

      ut::expect(harmony::validate(opt));
    }
    {
      std::vector<int> v{};

      ut::expect(not harmony::validate(v));

      v.push_back(10);

      ut::expect(harmony::validate(v));
    }
  };

  "cpo unit test"_test = [] {
    {
      int n = 0;
      int *p = &n;

      harmony::unit(p, 10);

      !ut::expect(harmony::validate(p));
      10_i == harmony::unwrap(p);

      int m = 20;
      harmony::unit(p, &m);

      !ut::expect(harmony::validate(p));
      20_i == harmony::unwrap(p);
    }
    {
      std::optional<int> opt = 1;

      harmony::unit(opt, 10);

      !ut::expect(harmony::validate(opt));
      10_i == harmony::unwrap(opt);

      harmony::unit(opt, std::optional<int>{20});

      !ut::expect(harmony::validate(opt));
      20_i == harmony::unwrap(opt);
    }
  };

  "cpo unwrap_other test"_test = [] {
    {
      int* p = nullptr;

      int* n = harmony::unwrap_other(p);

      ut::expect(n == nullptr);
    }
    {
      std::optional<int> opt{};

      std::optional<int> n = harmony::unwrap_other(opt);

      ut::expect(n == std::nullopt);
    }
    {
      simple_result<int, std::string> res{std::string("test either")};

      auto &str = harmony::unwrap_other(res);

      ut::expect(str == "test either");
    }
  };

  "type monas test"_test = [] {
    ut::expect(harmony::unwrappable<harmony::monas<std::optional<int> &>>);
    ut::expect(harmony::maybe<harmony::monas<std::optional<int> &>>);
    ut::expect(harmony::either<harmony::monas<std::optional<int> &>>);
    ut::expect(harmony::unwrappable<harmony::monas<std::optional<int>>>);
    ut::expect(harmony::maybe<harmony::monas<std::optional<int>>>);
    ut::expect(harmony::either<harmony::monas<std::optional<int>>>);
    ut::expect(harmony::unwrappable<harmony::monas<int*>>);
    ut::expect(harmony::maybe<harmony::monas<int*>>);
    ut::expect(harmony::either<harmony::monas<int*>>);

    std::optional<int> opt = 10;

    auto m = harmony::monas(opt);

    !ut::expect(harmony::validate(m));
    ut::expect(*opt == *m);

    auto&& m2 = ~m | [](int n) { return n + n;};

    !ut::expect(harmony::validate(m2));
    20_i == *m2;
  };

  "monas bind test"_test = [] {
    {
      std::optional<int> opt = 10;

      // 暗黙変換可能
      std::optional<int> result = harmony::monas(opt) | [](int n) { return std::optional<int>{n + n}; } 
                                                      | [](int n) { return std::optional<int>{n + 100}; };

      !ut::expect(harmony::validate(result));
      120_i == harmony::unwrap(result);
      // 状態は起点のオブジェクトに伝搬する
      ut::expect(opt == result);
    }
    {
      std::optional<int> opt = 10;
  
      // 途中で失敗する処理のチェーン
      std::optional<int> result = harmony::monas(opt)
        | [](int n) { return std::optional<int>{ n + n }; }
        | [](int n) { return n + 100;}
        | [](int)   { return std::nullopt;}
        | [](int n) { return n*n;};

      ut::expect(not harmony::validate(result));
      // 状態は起点のオブジェクトに伝搬する
      ut::expect(opt == result);
    }
    {
      // 右辺値からスタート
      std::optional<int> result = harmony::monas(std::optional<int>{10}) 
        | [](int n) { return std::optional<int>{n + n}; } 
        | [](int n) { return std::optional<int>{n + 100}; };

      !ut::expect(harmony::validate(result));
      120_i == harmony::unwrap(result);
    }
    {
      int n = 10;
      int* p = &n;

      auto m = harmony::monas(p) 
        | [](int n) { return n + n; } 
        | [](int n) { return n + 100; };

      !ut::expect(harmony::validate(m));
      120_i == *m;
      // 状態は起点のオブジェクトに伝搬する
      ut::expect(n == *m);

      int* r = ~m | [](int) { return 0; }
                  | [](int) { return nullptr;}
                  | [](int) { return 1; };

      !ut::expect(not harmony::validate(r));
      // 状態は起点のオブジェクトに伝搬する
      ut::expect(r == p);
      // 参照元には失敗直前の結果が入る
      0_i == n;
    }
    {
      auto r = harmony::monas(std::vector<int>{1, 2, 3, 4, 5})
        | [](int n) { return 2*n; }
        | [](int n) { return n + 1;};

      !ut::expect(harmony::validate(r));

      ut::expect(std::vector<int>{3, 5, 7, 9, 11} == harmony::unwrap(r));
    }
  };

  "map test"_test = [] {
    using namespace harmony::monadic_op;
    {
      int n = 10;

      auto opt = harmony::monas(&n) 
        | [](int n) { return n + n; }
        | [](int n) { return n + 100; }
        | map([](int n) { return float(n) + 0.1f;})
        | map([](float f) { return double(f);})
        | [](double d) { return d + d;}
        | transform([](double d) { return std::optional<double>{d + 0.01};})
        | [](double d) { return std::ceil(d * 100.0); };

      !ut::expect(harmony::validate(opt));
      24021.0_d == harmony::unwrap(opt);
    }
    {
      auto sum = harmony::monas(std::vector<int>{1, 2, 3, 4, 5})
        | [](int n) { return 2*n; }
        | [](int n) { return n + 1;}
        | map([](auto& vec) {
          int s{};
          for (int n : vec) {
            s += n;
          }
          return s;
        })
        | to_value<int>;

      35_i == sum;
    }
  };

  "and_then test"_test = []() {
    using namespace harmony::monadic_op;

    auto opt = harmony::monas(std::optional<int>{10}) 
      | [](int n) { return n + n; }
      | and_then([](int n) { return std::optional<int>{n + 100}; })
      | [](int n) { return ++n;}
      | and_then([](int n) { return std::optional<double>(double(n)); });

    !ut::expect(harmony::validate(opt));
    121.0_d == harmony::unwrap(opt);

    auto fail = ~opt
      | [](double) { return std::nullopt; }
      | and_then([](double d) { assert(false); return std::optional<double>(++d); });

    ut::expect(not harmony::validate(fail));
  };
}