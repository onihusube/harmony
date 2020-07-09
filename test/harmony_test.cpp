#include "boost/ut.hpp"
#include "harmony.hpp"

#include <optional>
#include <vector>
#include <any>

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
    }
    {
      std::optional<int> opt = 1;

      harmony::unit(opt, 10);

      !ut::expect(harmony::validate(opt));
      10_i == harmony::unwrap(opt);
    }
  };

  "type monas test"_test = [] {
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
        | [](int n) { return std::optional<int>{ n + 100 };}
        | [](int)   { return std::nullopt;}
        | [](int n) { return n*n;};

      ut::expect(not harmony::validate(result));
      // 状態は起点のオブジェクトに伝搬する
      ut::expect(opt == result);
    }
  };

  //"operation bind test"_test = []{};
}