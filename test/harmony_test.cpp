#include <optional>
#include <vector>

#include "boost/ut.hpp"
#include "harmony.hpp"

namespace ut = boost::ut;

int main() {
  using namespace boost::ut::literals;
  using namespace boost::ut::operators::terse;

  "concept unwrappable test"_test = [] {
    ut::expect(harmony::unwrappable<int*>);
    ut::expect(harmony::unwrappable<std::optional<int>>);
    ut::expect(harmony::unwrappable<std::vector<int>>);
  };

  "concept maybe test"_test = [] {
    ut::expect(harmony::maybe<int *>);
    ut::expect(harmony::maybe<std::optional<int>>);
    ut::expect(harmony::maybe<std::vector<int>>);
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

  "type monas test"_test = [] {
    ut::expect(harmony::unwrappable<harmony::monas<std::optional<int> &>>);
    ut::expect(harmony::maybe<harmony::monas<std::optional<int> &>>);

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
  };
}