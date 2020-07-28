#include <optional>
#include <vector>
#include <cmath>
#include <string>
#include <memory>
#include <list>
#include <array>

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning(disable : 4459)
#endif // _MSC_VER

#include "boost/ut.hpp"

#ifdef _MSC_VER
#pragma warning( pop )
#endif // _MSC_VER

#include "expected.hpp"
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
    , is_ok_v{false}
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

  "concept unwrappable test"_test = [] {
    ut::expect(harmony::unwrappable<int*>);
    ut::expect(harmony::unwrappable<std::optional<int>>);
    ut::expect(harmony::unwrappable<std::vector<int>>);
    ut::expect(harmony::unwrappable<std::unique_ptr<int>>);
    ut::expect(harmony::unwrappable<std::shared_ptr<int>>);
    ut::expect(harmony::unwrappable<simple_result<int, std::string>>);
    ut::expect(harmony::unwrappable<tl::expected<int, std::string>>);
  };

  "concept maybe test"_test = [] {
    ut::expect(harmony::maybe<int *>);
    ut::expect(harmony::maybe<std::optional<int>>);
    ut::expect(harmony::maybe<std::vector<int>>);
    ut::expect(harmony::maybe<std::unique_ptr<int>>);
    ut::expect(harmony::maybe<std::shared_ptr<int>>);
    ut::expect(harmony::maybe<simple_result<int, std::string>>);
    ut::expect(harmony::maybe<tl::expected<int, std::string>>);
  };

  "concept list test"_test = [] {
    ut::expect(not harmony::list<int *>);
    ut::expect(not harmony::list<std::optional<int>>);
    ut::expect(harmony::list<std::vector<int>>);
    ut::expect(harmony::list<std::list<int>>);
    ut::expect(harmony::list<std::array<int, 5>>);
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
    ut::expect(harmony::rewrappable<tl::expected<int, std::string>, int>);
    ut::expect(not harmony::rewrappable<tl::expected<int, std::string>, std::string>);
  };

  "concept either test"_test = [] {
    ut::expect(harmony::either<int*>);
    ut::expect(harmony::either<std::optional<int>>);
    ut::expect(harmony::either<std::unique_ptr<int>>);
    ut::expect(harmony::either<std::shared_ptr<int>>);
    ut::expect(harmony::either<simple_result<int, std::string>>);
    ut::expect(harmony::either<tl::expected<int, std::string>>);
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
    {
      tl::expected<int, std::string> ex{10};

      auto r = harmony::monas(ex)
        | [](int n) { return 2*n; }
        | [](int n) { return n + 1;};

      ut::expect(harmony::validate(r));
      // 状態は起点のオブジェクトに伝搬する
      ut::expect(*r == ex);
      21_i == harmony::unwrap(r);
    }
  };

  "then test"_test = [] {
    using namespace harmony::monadic_op;
    {
      std::optional<int> opt = 10;

      // 暗黙変換可能
      std::optional<int> result = opt
        | then([](int n) { return std::optional<int>{n + n}; })
        | then([](int n) { return std::optional<int>{n + 100}; });

      !ut::expect(harmony::validate(result));
      120_i == harmony::unwrap(result);
      // 状態は起点のオブジェクトに伝搬する
      ut::expect(opt == result);
    }
    {
      std::optional<int> opt = 10;

      // 途中で失敗する処理のチェーン
      std::optional<int> result = opt
        | then( [](int n) { return std::optional<int>{ n + n }; })
        | [](int n) { return n + 100; }
        | [](int) { return std::nullopt; }
        | [](int n) { return n * n; };

      ut::expect(not harmony::validate(result));
      // 状態は起点のオブジェクトに伝搬する
      ut::expect(opt == result);
    }
    {
      int n = 10;
      int* p = &n;

      auto m = p
        | then([](int n) { return n + n; })
        | [](int n) { return n + 100; };

      !ut::expect(harmony::validate(m));
      120_i == *m;
      // 状態は起点のオブジェクトに伝搬する
      ut::expect(n == *m);
    }
    {
      auto r = std::vector<int>{1, 2, 3, 4, 5}
        | then([](int n) { return 2 * n; })
        | [](int n) { return n + 1; };

      !ut::expect(harmony::validate(r));

      ut::expect(std::vector<int>{3, 5, 7, 9, 11} == harmony::unwrap(r));
    }
    {
      tl::expected<int, std::string> ex{ 10 };

      auto r = ex
        | then([](int n) { return 2 * n; })
        | [](int n) { return n + 1; };

      ut::expect(harmony::validate(r));
      // 状態は起点のオブジェクトに伝搬する
      ut::expect(*r == ex);
      21_i == harmony::unwrap(r);
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
      auto opt = harmony::monas(std::optional<int>{10}) 
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
        | map([](std::vector<int>& vec /*ここをautoで受けるようにするとMSVCはこける*/) {
            int s{};
            for (int n : vec) {
              s += n;
            }
            return s;
          })
        | map_to<int>;

      35_i == sum;
    }

#ifndef _MSC_VER
    {
      using namespace std::string_view_literals;
      tl::expected<int, std::string> ex{10};

      auto r = harmony::monas(ex)
        | [](int n) { return 2*n; }
        | [](int n) { return n + 1;}
        | map([](int n) { return double(n);})
        | [](double d) { return ++d; }
        | map([](double d) { return std::to_string(d);});

      ut::expect(harmony::validate(r));
      "22.0"sv == harmony::unwrap(r);
    }
#endif // !_MSC_VER
  };

  "map_err test"_test = [] {
    using namespace harmony::monadic_op;
    {
      int n = 10;

      auto r = harmony::monas(&n) 
        | [](int n) { return n + n; }
        | [](int n) { return n + 100; }
        | map_err([](std::nullptr_t) {assert(false); return nullptr; })
        | map([](int) { return std::optional<int>{};})
        | map_err([](std::nullopt_t) { return false;})
        | map([](int v) { assert(false); return v; });

      !ut::expect(not harmony::validate(r));
      ut::expect(harmony::unwrap_other(r) == false);
    } 
    {
      auto r = harmony::monas(std::optional<int>{10}) 
        | [](int n) { return n + n; }
        | [](int) { return std::nullopt; }
        | map_err([](std::nullopt_t) { return nullptr;})
        | map_err([](std::nullptr_t) { return false;})
        | map([](int n) { assert(false); return n; })
        | map_err([](bool b) { return std::optional{b};})
        | map_err([](std::nullopt_t) { assert(false); return std::optional<bool>{true};});

      !ut::expect(harmony::validate(r));
      ut::expect(harmony::unwrap(r) == false);
    }
#ifndef _MSC_VER
    {
      using namespace std::string_view_literals;

      tl::expected<int, std::string> ex{10};

      auto r = harmony::monas(ex)
        | [](int n) { return 2*n; }
        | [](int) { return tl::expected<int, std::string>{tl::unexpect, "fail test"};}
        | map_err([](std::string str) { str.append(" map_err"); return str; })  // ここのmap_errでMSVCはこける
        | map_err([](std::string str) { return str == "fail test map_err"sv;})
        | map([](int n) { assert(false); return n;});

      !ut::expect(not harmony::validate(r));
      ut::expect(harmony::unwrap_other(r) == true);
    }
#endif // !_MSC_VER
  };

  "and_then test"_test = []() {
    using namespace harmony::monadic_op;

    {
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
    }

#ifndef _MSC_VER
    {
      using namespace std::string_view_literals;

      tl::expected<int, std::string> ex{10};

      auto r = harmony::monas(ex)
        | and_then([](int n) { return tl::expected<int, std::string>{2 * n}; })
        | and_then([](int n) { return tl::expected<double, std::string>{double(n)};});

      !ut::expect(harmony::validate(r));
      20.0_d == harmony::unwrap(r);

      ex = 20;
      auto r2 = harmony::monas(ex)
        | and_then([](int) { return tl::expected<int, std::string>{ tl::unexpect, "failed!"}; })
        | and_then([](int) { assert(false); return tl::expected<int, std::string>{10};});

      !ut::expect(not harmony::validate(r2));
      ut::expect("failed!"sv == harmony::unwrap_other(r2));
    }
#endif // !_MSC_VER
  };

  "or_else test"_test = [] {
    using namespace harmony::monadic_op;
    {
      auto opt = harmony::monas(std::optional<int>{10}) 
        | [](int) { return std::nullopt; }
        | and_then([](int n) { return std::optional<int>{n + 100}; })
        | or_else([](auto) { return std::optional<double>{1.0};})
        | [](double d) { return 2.0 * d;};

      !ut::expect(harmony::validate(opt));
      2.0_d == harmony::unwrap(opt);

      auto success = ~opt
        | or_else([](auto) { assert(false); return std::optional<double>(1.0); })
        | or_else([](auto) { assert(false); return std::optional<double>(3.0); });

      ut::expect(harmony::validate(success));
      2.0_d == harmony::unwrap(success);
    }

#ifndef _MSC_VER
    {
      using namespace std::string_view_literals;

      tl::expected<int, std::string_view> ex{tl::unexpect, "expected failed test"};

      auto r = harmony::monas(ex)
        | or_else([](std::string_view str) { return tl::expected<int, std::string_view>{tl::unexpect, str.substr(0, 15)}; })
        | or_else([](std::string_view str) { return tl::expected<int, std::string_view>{tl::unexpect, str.substr(9, 6)}; });

      !ut::expect(not harmony::validate(r));
      ut::expect("failed"sv == harmony::unwrap_other(r));

      auto r2 = harmony::monas(tl::expected<int, std::string_view>{tl::unexpect, "failed"sv})
        | or_else([](std::string_view) { return tl::expected<int, std::string_view>{20}; })
        | or_else([](std::string_view) { assert(false); return tl::expected<int, std::string_view>{0};});

      !ut::expect(harmony::validate(r2));
      20_i == harmony::unwrap(r2);
    }
#endif // !_MSC_VER
  };

  "match test"_test = [] {
    using namespace harmony::monadic_op;

    {
      int n = 10;

      int r = harmony::monas(&n)
        | match([](int n){ return 2*n;}, [](std::nullptr_t) { assert(false); return 0;});

      20_i == r;

      int *p = nullptr;

      r = harmony::monas(p)
        | match([](int){ assert(false); return 0;}, [](std::nullptr_t) { return 1;});
      
      1_i == r;
    }
    {
      int r = harmony::monas(std::optional<int>{10})
        | match([](int n){ return 2*n;}, [](std::nullopt_t) { assert(false); return 0;});

      20_i == r;

      r = harmony::monas(std::optional<int>{})
        | fold([](int){ assert(false); return 0;}, [](std::nullopt_t) { return 1;});
      
      1_i == r;
    }

#ifndef _MSC_VER
    {
      using namespace std::string_view_literals;

      tl::expected<double, int> ex{3.14};

      // 戻り値のstringはモナド的な型と見なされるのでmonasでラップされてる（ここでは明示的な暗黙変換により取り出している）
      std::string str = ex | fold([](double d) { return std::to_string(d); }, [](int n) { return std::to_string(-n); });

      ut::expect(str == "3.140000"sv);

      tl::expected<double, int> ex2{tl::unexpect, 3};
      std::string str2 = ex2 | match([](double d) { return std::to_string(d); }, [](int n) { return std::to_string(-n); });
      ut::expect(str2 == "-3"sv);
    }
    // 1引数match
    {
      using namespace std::string_view_literals;

      std::string str = tl::expected<double, int>{3.14}
        | match([](auto v) { return std::to_string(v); });

      ut::expect(str == "3.140000"sv);

      std::string str2 = tl::expected<double, int>{tl::unexpect, 3}
        | fold([](auto v) { return std::to_string(v); });

      ut::expect(str2 == "3"sv);
    }
#endif // !_MSC_VER

    // 結果が再びモナド的な型となるmatch
    {
      int n = 10;

      auto suc = harmony::monas(&n)
        | match([](int n){ return std::optional<int>{2*n};}, [](std::nullptr_t) { return std::nullopt;})
        | [](int n) { return n + n;}
        | map([](int n) { return double(n);});

      !ut::expect(harmony::validate(suc));
      40.0_d == harmony::unwrap(suc);

      int *p = nullptr;

      auto fail = harmony::monas(p)
        | match([](int n){ return std::optional<int>{2*n};}, [](std::nullptr_t) { return std::nullopt;})
        | [](int n) {return n + 10;}
        | map_err([](std::nullopt_t) { return -1; });

      !ut::expect(not harmony::validate(fail));
      -1_i == harmony::unwrap_other(fail);
    }
    // 結果がvoidとなるmatch
    {
      int n = 10;

      int r = 0;
      harmony::monas(&n)
        | match([&r](int n){ r = n;}, [](std::nullptr_t) { assert(false);});

      10_i == r;

      int *p = nullptr;

      harmony::monas(p)
        | match([](int){ assert(false);}, [&r](std::nullptr_t) { r = -1; });
      
      -1_i == r;
    }
  };

  "exists test"_test = [] {
    using namespace harmony::monadic_op;

    {
      int n = 10;

      bool r = harmony::monas(&n)
        | [](int n) { return n + n; }
        | exists([](int n) { return n == 20;});

      ut::expect(r == true);

      r = harmony::monas(&n)
        | exists([](int n) { return n == 10;});

      ut::expect(r == false);

      int *p = nullptr;

      r = harmony::monas(p)
        | [](int n) { return n + n; }
        | exists([](int n) { return n == 0;});

      ut::expect(r == false);
    }
    //シーケンスに対するexists
    {
      std::vector<int> vec = {2, 4, 6, 8, 10};

      bool r = vec | exists([](int n) { return n == 8; });

      ut::expect(r == true);

      r = vec | exists([](int n) { return n % 2 == 1; });

      ut::expect(r == false);
    }
  };

  "try_catch test"_test = [] {
    using namespace harmony::monadic_op;
    using namespace std::string_view_literals;

    auto f = [](int n, int m) -> int {
      if (m == 0) throw "division by zero";
      return n / m;
    };

    // 例外を投げない処理
    bool r = try_catch(f, 4, 2)
      | map([](int n) { return n == 2; })
      | map_err([](auto exptr) { assert(false); return exptr;})
      | map_to<bool>;

    ut::expect(r == true);

    // 例外を投げる処理
    auto str = try_catch(f, 4, 0)
      | map([](int) { assert(false); return std::string{}; })
      | map_err([](std::exception_ptr exptr) { 
          try { std::rethrow_exception(exptr); }
          catch(const char* message) {
            return std::string{message};
          }
        })
      | fold_to<std::string>;

    ut::expect(str == "division by zero"sv);
  };
}
