#include "harmony.hpp"

#include <optional>
#include <vector>
#include <cmath>
#include <string>
#include <memory>
#include <list>
#include <array>
#include <future>
#include <system_error>
#include <numbers>

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning(disable : 4459)
#endif // _MSC_VER

#define BOOST_UT_DISABLE_MODULE

#include "boost/ut.hpp"

#ifdef _MSC_VER
#pragma warning( pop )
#endif // _MSC_VER

#include "expected.hpp"

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

template<typename T>
struct simple_unwrappable {
  T t;

  auto unwrap() const noexcept -> const T& {
    return t;
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
    ut::expect(harmony::unwrappable<std::variant<int, double>>);
    ut::expect(harmony::unwrappable<simple_result<int, std::string>>);
    ut::expect(harmony::unwrappable<tl::expected<int, std::string>>);
    ut::expect(harmony::unwrappable<std::future<int>>);
    ut::expect(harmony::unwrappable<std::future<int&>>);
    ut::expect(not harmony::unwrappable<std::future<void>>);
    ut::expect(harmony::unwrappable<std::shared_future<int>>);
    ut::expect(harmony::unwrappable<std::shared_future<int&>>);
    ut::expect(not harmony::unwrappable<std::shared_future<void>>);
  };

  "concept maybe test"_test = [] {
    ut::expect(harmony::maybe<int *>);
    ut::expect(harmony::maybe<std::optional<int>>);
    ut::expect(harmony::maybe<std::vector<int>>);
    ut::expect(harmony::maybe<std::unique_ptr<int>>);
    ut::expect(harmony::maybe<std::shared_ptr<int>>);
    ut::expect(harmony::maybe<std::variant<int, double>>);
    ut::expect(harmony::maybe<simple_result<int, std::string>>);
    ut::expect(harmony::maybe<tl::expected<int, std::string>>);
    ut::expect(harmony::maybe<std::future<int>>);
    ut::expect(harmony::maybe<std::future<int &>>);
    ut::expect(not harmony::maybe<std::future<void>>);
    ut::expect(harmony::maybe<std::shared_future<int>>);
    ut::expect(harmony::maybe<std::shared_future<int &>>);
    ut::expect(not harmony::maybe<std::shared_future<void>>);
    ut::expect(harmony::maybe<std::error_code>);
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
    ut::expect(harmony::either<std::variant<int, double>>);
    ut::expect(harmony::either<simple_result<int, std::string>>);
    ut::expect(harmony::either<tl::expected<int, std::string>>);
    ut::expect(not harmony::either<std::error_code>);
  };

  "concept maybe_of"_test = [] {
    ut::expect(harmony::maybe_of<int*, int&>);
    ut::expect(harmony::maybe_of<std::optional<int>, int&&>);
    ut::expect(harmony::maybe_of<std::vector<int>, std::ranges::views::all_t<std::vector<int>&>>);
    ut::expect(harmony::maybe_of<std::unique_ptr<int>, int&>);
    ut::expect(harmony::maybe_of<std::shared_ptr<int>, int&>);
    ut::expect(harmony::maybe_of<std::variant<int, double>, double&&>);
    ut::expect(harmony::maybe_of<simple_result<int, std::string>, int&>);
    ut::expect(harmony::maybe_of<tl::expected<int, std::string>, int&&>);
    ut::expect(harmony::maybe_of<std::future<int>, std::variant<std::exception_ptr, int>>);
    ut::expect(harmony::maybe_of<std::future<int &>, std::variant<std::exception_ptr, std::reference_wrapper<int>>>);
    ut::expect(harmony::maybe_of<std::shared_future<int>, std::variant<std::exception_ptr, std::reference_wrapper<const int>>>);
    ut::expect(harmony::maybe_of<std::shared_future<int &>, std::variant<std::exception_ptr, std::reference_wrapper<int>>>);
    ut::expect(harmony::maybe_of<std::error_code, int>);
  };

  "concept either_of"_test = [] {
    ut::expect(harmony::either_of<int *, std::nullptr_t, int &>);
    ut::expect(harmony::either_of<std::optional<int>, std::nullopt_t, int&&>);
    ut::expect(harmony::either_of<std::unique_ptr<int>, std::nullptr_t, int &>);
    ut::expect(harmony::either_of<std::shared_ptr<int>, std::nullptr_t, int &>);
    ut::expect(harmony::either_of<std::variant<int, double>, int&&, double&&>);
    ut::expect(harmony::either_of<simple_result<int, std::string>, std::string&, int&>);
    ut::expect(harmony::either_of<tl::expected<int, std::string>, std::string&&, int&&>);
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
    {
      std::variant<int, std::string> v{std::in_place_index<1>, "test either"};

      auto &str = harmony::unwrap(v);

      ut::expect(str == "test either");
    }
    {
      auto f = std::async([]{return 10;});

      auto future_either = harmony::unwrap(f);

      10_i == harmony::unwrap(future_either);

      using either_t = decltype(future_either);
      ut::expect(std::same_as<std::variant<std::exception_ptr, int>, either_t>);
    }
    {
      std::future<int> f{};

      auto future_either = harmony::unwrap(f);

      try { 
        std::rethrow_exception(harmony::unwrap_other(future_either)); 
      } catch (const std::exception&) {
        ut::expect(true);
      } catch (...) {
        ut::expect(false);
      }
    }
    {
      int n = 20;
      auto f = std::async([&n]() mutable -> int& { return n; });

      auto future_either = harmony::unwrap(f);

      20_i == harmony::unwrap(future_either).get();

      using either_t = decltype(future_either);
      ut::expect(std::same_as<std::variant<std::exception_ptr, std::reference_wrapper<int>>, either_t>);
    }
    {
      auto f = std::async([]{return 10;});

      // shared_future取得
      auto sf = f.share();

      auto future_either = harmony::unwrap(sf);

      10_i == harmony::unwrap(future_either).get();

      using either_t = decltype(future_either);
      ut::expect(std::same_as<std::variant<std::exception_ptr, std::reference_wrapper<const int>>, either_t>);
    }
    {
      int ec_int = static_cast<int>(std::errc::bad_address);
      std::error_code ec{ec_int, std::system_category()};

      ut::expect(harmony::unwrap(ec) == ec_int);
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
    {
      std::variant<int, std::string> v{std::in_place_index<0>, 20};

      ut::expect(not harmony::validate(v));

      std::variant<int, std::string> v2{std::in_place_index<1>, "str"};

      ut::expect(harmony::validate(v2));
    }
    {
      auto f = std::async([] { return 10; });

      ut::expect(harmony::validate(f));

      [[maybe_unused]] auto future_either = harmony::unwrap(f);

      ut::expect(not harmony::validate(f));
    }
    {
      auto f = std::async([] { return 10; });

      ut::expect(harmony::validate(f));

      // shared_future取得
      auto sf = f.share();

      ut::expect(not harmony::validate(f));

      ut::expect(harmony::validate(sf));

      [[maybe_unused]] auto future_either = harmony::unwrap(sf);

      // shared_futureはget()の後でも共有状態を維持している
      ut::expect(harmony::validate(sf));
    }
    {
      std::error_code ec{};

      ut::expect(not harmony::validate(ec));

      ec.assign(int(std::errc::bad_address), std::system_category());

      ut::expect(harmony::validate(ec));
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
    {
      std::variant<int, std::string> v{std::in_place_index<0>, 20};

      20_i == harmony::unwrap_other(v);
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

      int arr[] = {3, 5, 7, 9, 11};
      ut::expect(std::ranges::equal(arr, harmony::unwrap(r)));
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
    {
      // voidを返す関数を与えるテスト1

      std::optional<int> opt = 10;
      int se = 0;
  
      std::optional<int> result = harmony::monas(opt)
        | [&se](int) { ++se; }
        | [&se](int) { ++se; };

      ut::expect(harmony::validate(result));

      ut::expect(*result == 10);
      ut::expect(result == opt);
      ut::expect(se == 2);
    }
    {
      // voidを返す関数を与えるテスト2

      simple_unwrappable su{ .t = 10 };
      int se = 0;

      harmony::monas(su)
        | [&se](int) { ++se; }
        | [&se](int) { ++se; };
      
      ut::expect(su.t == 10);
      ut::expect(se == 2);
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

      int arr[] = {3, 5, 7, 9, 11};
      ut::expect(std::ranges::equal(arr, harmony::unwrap(r)));
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
        | map([](auto ref_view) {
            int s{};
            for (int n : ref_view) {
              s += n;
            }
            return s;
          })
        | map_to<int>;

      35_i == sum;
    }

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
      ut::expect(harmony::unwrap(r) == "22.000000"sv);
    }
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

  "future test"_test = [] {
    using namespace harmony::monadic_op;
    using namespace std::chrono_literals;
    using namespace std::string_view_literals;

    // 例外を投げない
    {
      auto str = std::async([]() {
                   std::this_thread::sleep_for(100ms);
                   return 20;
                 }) | then([](int n) { return n * 10; })
                    | map([](int n) { return std::to_string(n);})
                    | map_err([](auto) { return std::to_string(-1);})
                    | fold_to<std::string>;

      ut::expect(str == "200"sv);
    }
    // 例外を投げる
    {
      auto str = std::async([]() -> int {
                   std::this_thread::sleep_for(100ms);
                   throw std::runtime_error("error!!");
                 }) | then([](int n) { return n * 10; })
                    | map([](int n) { return std::to_string(n);})
                    | map_err([](auto exptr) {
                        try {
                          std::rethrow_exception(exptr);
                        } catch (const std::exception& ex) {
                          return std::string{ex.what()};
                        } catch (...) {
                          return std::to_string(-1);
                        }
                      })
                    | fold_to<std::string>;

      ut::expect(str == "error!!"sv);
    }
    // 空のfuture
    {
      std::future<int> fu{};

      harmony::specialization_of<harmony::monas> auto fm = fu | then([](int n) { return n * 10; });

      ut::expect(harmony::validate(fm) == false);

      auto str = fm | map([](int n) { return std::to_string(n);})
                    | map_err([](auto exptr) {
                        try {
                          std::rethrow_exception(exptr);
                        } catch (const std::future_error&) {
                          return std::string{"std::future_error"};
                        } catch (...) {
                          return std::string{"unknown"};
                        }
                      })
                    | fold_to<std::string>;

      ut::expect(str == "std::future_error"sv);
    }
  };

  "value_or"_test = [] {
    {
      std::optional<int> opt{10};

      std::integral auto n = harmony::monas(opt) | harmony::value_or(100);

      ut::expect(n == 10_i);
    }
    {
      std::optional<int> opt{};

      std::integral auto n = harmony::monas(opt) | harmony::value_or(100);

      ut::expect(n == 100_i);
    }
    {
      tl::expected<double, int> exp{3.14};

      std::floating_point auto d = harmony::monas(exp) | harmony::value_or(std::numbers::phi);
      ut::expect(d == 3.14);
    }
    {
      tl::expected<double, int> exp{tl::unexpect, 2};

      std::floating_point auto d = harmony::monas(exp) | harmony::value_or(std::numbers::phi);
      ut::expect(d == std::numbers::phi);
    }
    {
      double v = 3.14;
      double *p = &v;

      std::floating_point auto d = harmony::monas(p) | harmony::value_or(std::numbers::phi);
      ut::expect(d == 3.14_d);
    }
    {
      double* p = nullptr;

      std::floating_point auto d = harmony::monas(p) | harmony::value_or(std::numbers::phi);
      ut::expect(d == std::numbers::phi);
    }
  };

  "value_or_construct"_test = [] {
    {
      tl::expected<double, int> exp{3.14};

      std::floating_point auto d = harmony::monas(exp) | harmony::value_or_construct(std::numbers::phi);
      ut::expect(d == 3.14_d);
    }
    {
      tl::expected<double, int> exp{tl::unexpect, 2};

      std::floating_point auto d = harmony::monas(exp) | harmony::value_or_construct(std::numbers::phi);
      ut::expect(d == std::numbers::phi);
    }

    struct C {
      int n;
      double d;

      C() : n(1), d{1.0} {}

      C(int a, int b, int c) : n(a+b+c), d{} {}

      C(int a, double b) : n{a}, d{b} {}
    };

    {
      std::optional<C> opt{C{}};

      std::same_as<C> auto c = harmony::monas(opt) | harmony::value_or_construct(0, 1, 2);
      ut::expect(c.n == 1_i);
      ut::expect(c.d == 1.0_d);
    }
    {
      std::optional<C> opt{};

      std::same_as<C> auto c = harmony::monas(opt) | harmony::value_or_construct(0, 1, 2);
      ut::expect(c.n == 3_i);
      ut::expect(c.d == 0.0_d);
    }
    {
      std::optional<C> opt{};

      std::same_as<C> auto c = harmony::monas(opt) | harmony::value_or_construct(3, 3.14);
      ut::expect(c.n == 3_i);
      ut::expect(c.d == 3.14_d);
    }
  };

  "invert"_test = [] {
    {
      std::optional<int> opt{10};
      auto abekobe = harmony::invert(opt) | [](std::nullopt_t) { ut::expect(true); };
      
      ut::expect(not harmony::validate(abekobe));
      ut::expect(harmony::unwrap_other(abekobe) == 10);
    }

    {
      std::error_code ec{};

      auto abekobe1 = harmony::invert(ec) | [](std::error_code& ec_a) { ut::expect(not ec_a); };

      ut::expect(harmony::validate(abekobe1));
      ut::expect(harmony::unwrap_other(abekobe1) == 0);

      int ec_int = static_cast<int>(std::errc::bad_address);
      ec.assign(ec_int, std::system_category());

      auto abekobe2 = harmony::invert(ec) | [](std::error_code&) { ut::expect(false); };

      ut::expect(not harmony::validate(abekobe2));
      ut::expect(harmony::unwrap_other(abekobe2) == ec_int);
    }

  };

  "harmonize"_test = []{
    using namespace harmony::monadic_op;

    {
      bool boolean = true;

      auto ret = harmony::harmonize(boolean, false) 
        | [](bool b) { ut::expect(b); }
        | fold_to<bool>;
      
      ut::expect(ret);

      boolean = false;

      auto ret2 = harmony::harmonize(boolean, false) 
        | [](bool) { ut::expect(false); }
        | fold_to<bool>;

      ut::expect(not ret2);
    }

    {
      // boolオーバーロードのテスト

      bool boolean = true;

      auto ret = harmony::harmonize(boolean) 
        | [](bool b) { ut::expect(b); }
        | fold_to<bool>;
      
      ut::expect(ret);

      boolean = false;

      auto ret2 = harmony::harmonize(boolean) 
        | [](bool) { ut::expect(false); }
        | fold_to<bool>;

      ut::expect(not ret2);
    }

    {
      float vf = 1.0f;
      constexpr float nan = std::numeric_limits<float>::quiet_NaN();

      auto ret = harmony::harmonize(vf, [](float f) { return std::isnan(f);}) 
        | [](float f) { ut::expect(f == 1.0f); }
        | fold_to<float>;

      ut::expect(ret = 1.0f);

      vf = nan;

      float ret2 = harmony::harmonize(vf, [](float f) { return std::isnan(f);}) 
        | [](float) { ut::expect(false); }
        | fold_to<float>;

      ut::expect(std::isnan(ret2));
    }

    {
      // 浮動小数点数型オーバーロードのテスト

      double vf = 1.0f;
      constexpr double nan = std::numeric_limits<double>::quiet_NaN();

      auto ret = harmony::harmonize(vf) 
        | [](double f) { ut::expect(f == 1.0f); }
        | fold_to<double>;

      ut::expect(ret = 1.0f);

      vf = nan;

      double ret2 = harmony::harmonize(vf) 
        | [](double) { ut::expect(false); }
        | fold_to<double>;

      ut::expect(std::isnan(ret2));
    }

    {
      std::int32_t n = 10;

      auto ret = harmony::harmonize(n, [](std::int32_t m) { return m < 0;}) 
        | [](int m) { ut::expect(m == 10); }
        | [](int m) { return ++m; }
        | fold_to<std::int32_t>;

      ut::expect(ret == 11);

      n = -1;

      auto ret2 = harmony::harmonize(n, [](std::int32_t m) { return m < 0;}) 
        | [](int) { ut::expect(false); }
        | [](int m) { return ++m; }
        | fold_to<std::int32_t>;

      ut::expect(ret2 == -1);
    }
  };
}
