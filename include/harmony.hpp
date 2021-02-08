#pragma once

#include <concepts>
#include <utility>
#include <ranges>
#include <cassert>
#include <variant>
#include <optional>
#include <functional>
#include <any>

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning(once : 4648)
#endif // _MSC_VER

#ifdef __EDG__
#include <ciso646>
#endif


namespace harmony::inline concepts {

  template<typename T>
  using with_reference = T&;

  /**
  * @brief 型Tがvoidではない
  */
  template<typename T>
  concept not_void = requires {
    typename with_reference<T>;
  };

  /**
  * @brief operator*による単純な間接参照が可能
  */
  template<typename T>
  concept weakly_indirectly_readable = requires(T&& t) {
    {*std::forward<T>(t)} -> not_void;
  };

  template<typename T>
  concept not_weakly_indirectly_readable = not weakly_indirectly_readable<T>;

  /**
  * @brief 縮小変換を起こさずにFrom -> Toへ変換可能
  */
  template<typename From, typename To>
  concept without_narrowing_convertible =
    std::convertible_to<From, To> and
    requires (From&& x) {
      { std::type_identity_t<To[]>{std::forward<From>(x)} } -> std::same_as<To[1]>;
    };

  namespace variant_like_detail {
    template <typename V>
    inline constexpr bool is_two_arguments_v = false;

    template <typename L, typename R, template <typename, typename> class V>
    inline constexpr bool is_two_arguments_v<V<L, R>> = true;

    using std::get;
  
    template<typename V>
    concept gettable = requires(V&& v) {
      get<0>(std::forward<V>(v));
      get<1>(std::forward<V>(v));
    };
  }

  template<typename V>
  concept variant_like = 
    variant_like_detail::is_two_arguments_v<std::remove_cvref_t<V>> and
    requires(V&& v) {
      {v.index()} -> std::integral;
    } and
    variant_like_detail::gettable<V>;
  
  namespace future_like_detail {
    template<typename F>
    inline constexpr bool is_one_argument_v = false;

    template<typename R, template <typename> class F>
    inline constexpr bool is_one_argument_v<F<R>> = true;
  }

  template<typename F>
  concept future_like =
    future_like_detail::is_one_argument_v<std::remove_cvref_t<F>> and
    requires(F& f, const F& cf) {
      {f.get()}  -> not_void;
      {cf.valid()} -> std::same_as<bool>;
    };

  namespace any_like_detail {
    template<typename T>
    concept pointer = std::is_pointer_v<T>;

    template<typename T>
    concept reference = std::is_reference_v<T>;

    template<typename A>
    concept any_cast_usable = requires(A& a, const A& ca, A&& ra) {
      {any_cast<int>(a)} -> not_void;
      {any_cast<int>(ca)} -> not_void;
      {any_cast<int>(std::move(ra))} -> not_void;
      {any_cast<int&>(a)} -> reference;
      {any_cast<int&>(ca)} -> reference;
      {any_cast<int>(&a)} -> pointer;
      {any_cast<int>(&ca)} -> pointer;
    };
  }
  
  template<typename A>
  concept any_like =
    requires(const A& any) {
      {any.type()} -> std::same_as<const std::type_info&>;
    } and 
    any_like_detail::any_cast_usable<A>;
}

namespace harmony::detail {

  template<typename T>
  concept value_func_usable = requires(T&& t) {
    {std::forward<T>(t).value()} -> not_void;
  };

  /**
  * @brief unwrap CPOの実装
  * @details 事前条件 : 各処理は取り出す値が存在している事を仮定する（言い換えると、validate()でチェック済みであること）
  */
  struct unwrap_impl {

    /**
    * @brief operator*によって値を取り出す
    */
    template<weakly_indirectly_readable T>
    [[nodiscard]]
    constexpr decltype(auto) operator()(T&& t) const noexcept {
      return *std::forward<T>(t);
    }

    /**
    * @brief value()メンバ関数によって値を取り出す
    */
    template<typename T>
      requires not_weakly_indirectly_readable<T> and
               value_func_usable<T>
    [[nodiscard]]
    constexpr decltype(auto) operator()(T&& t) const noexcept {
      return std::forward<T>(t).value();
    }

    /**
    * @brief unwrap()メンバ関数によって値を取り出す
    */
    template<typename T>
      requires not_weakly_indirectly_readable<T> and
               requires(T&& t) { {std::forward<T>(t).unwrap()} -> not_void; }
    [[nodiscard]]
    constexpr decltype(auto) operator()(T&& t) const noexcept {
      return std::forward<T>(t).unwrap();
    }

    /**
    * @brief range（listモナド）はref_viewで返す
    */
    template<std::ranges::range R>
    [[nodiscard]]
    constexpr auto operator()(R&& r) const noexcept {
      return std::views::all(r);
    }

    /**
    * @brief 2要素variantは2つ目の型の値を取得
    */
    template<variant_like V>
    [[nodiscard]]
    constexpr decltype(auto) operator()(V&& v) const noexcept {
      using std::get;
      return get<1>(std::forward<V>(v));
    }

    /**
    * @brief std::future/shread_futureと同等の型はget()関数で取得
    * @details future<void>はとりあえずリジェクト
    */
    template<future_like F>
    [[nodiscard]]
    constexpr auto operator()(F&& f) const {
      using R = decltype(std::forward<F>(f).get());
      using result_t = std::conditional_t<std::is_reference_v<R>
        , std::variant<std::exception_ptr, std::reference_wrapper<std::remove_reference_t<R>>>
        , std::variant<std::exception_ptr, R>>;

      try {
        return result_t(std::in_place_index<1>, std::forward<F>(f).get());
      } catch(...) {
        return result_t(std::in_place_index<0>, std::current_exception());
      }
    }

    template<not_void R, any_like A>
    decltype(auto) operator()(A&& any, std::in_place_type_t<R>) const {
      if constexpr (std::is_pointer_v<R>) {
        return any_cast<R>(&any);
      } else {
        return any_cast<R>(std::forward<A>(any));
      }
    }
  };

} // harmony::detail

namespace harmony::inline cpo {

  /**
  * @brief unwrap(a)のように呼び出し、任意の型の内包する値を取り出すカスタマイゼーションポイントオブジェクト
  */
  inline constexpr detail::unwrap_impl unwrap{};
  
}

namespace harmony::inline concepts {

  /**
  * @brief モナド的とみなせる型である
  */
  template<typename... T>
  concept unwrappable = requires(T&&... m) {
    { harmony::cpo::unwrap(std::forward<T>(m)...) } -> not_void;
  };

  /**
  * @brief bool型への明示的変換が可能
  */
  template<typename T>
  concept boolean_convertible = requires(const T& t) {
    bool(t);
  };

  template<typename T>
  concept not_boolean_convertible = not boolean_convertible<T>;

}

namespace harmony::detail {

  template<typename T>
  concept has_value_func_usable = requires(const T& t) {
    {t.has_value()} -> std::same_as<bool>;
  };

  /**
  * @brief validate CPOの実装
  */
  struct validate_impl {

    /**
    * @brief 明示的なbool変換によって状態を取得
    */
    template<unwrappable T>
      requires boolean_convertible<T>
    [[nodiscard]]
    constexpr bool operator()(const T& t) const noexcept(noexcept(bool(t))) {
      return bool(t);
    }

    /**
    * @brief has_value()メンバ関数によって状態を取得
    */
    template<unwrappable T>
      requires not_boolean_convertible<T> and
               has_value_func_usable<T>
    [[nodiscard]]
    constexpr bool operator()(const T& t) const noexcept(noexcept(t.has_value())) {
      return t.has_value();
    }

    /**
    * @brief is_ok()メンバ関数によって状態を取得
    */
    template<unwrappable T>
      requires not_boolean_convertible<T> and
               requires(const T& t) { {t.is_ok()} -> std::same_as<bool>; }
    [[nodiscard]]
    constexpr bool operator()(const T& t) const noexcept(noexcept(t.is_ok())) {
      return t.is_ok();
    }

    /**
    * @brief std::ranges::empty()CPOによって状態を取得
    */
    template<typename T>
      requires requires(const T& t) { {std::ranges::empty(t)} -> std::same_as<bool>; }
    [[nodiscard]]
    constexpr bool operator()(const T& t) const noexcept(noexcept(std::ranges::empty(t))) {
      return not std::ranges::empty(t);
    }

    /**
    * @brief 2要素variantはindex()メンバ関数で取得
    */
    template<variant_like V>
    [[nodiscard]]
    constexpr bool operator()(const V& v) const noexcept { 
      return v.index() == 1;
    }

    /**
    * @brief std::future/shread_futureと同等の型はvalid()関数で取得
    * @details こっちではfuture<void>も受け入れておく（一貫性のためにリジェクトする？
    */
    template<future_like F>
    [[nodiscard]]
    constexpr bool operator()(const F& f) const noexcept {
      return f.valid();
    }
  };
  
} // harmony::detail

namespace harmony::inline cpo {

  /**
  * @brief validate(a)のように呼び出し、モナド的な型が値を内包しているかをbool値で得るカスタマイゼーションポイントオブジェクト
  */
  inline constexpr detail::validate_impl validate{};
  
}

namespace harmony::inline concepts {

  /**
  * @brief meybeモナドである
  */
  template<typename T>
  concept maybe =
    unwrappable<T> and
    requires(const T& m) {
      { harmony::cpo::validate(m) } -> std::same_as<bool>;
    };
  
  /**
  * @brief listモナドである
  */
  template<typename T>
  concept list = maybe<T> and std::ranges::range<T>;

}

namespace harmony::traits {

  /**
  * @brief unwrap(a)の結果の型を得る
  * @tparam T unwrappableな型
  */
  template<typename T>
  using unwrap_t = decltype(harmony::cpo::unwrap(std::declval<T>()));
}

namespace harmony::detail {

  template<typename M, typename T>
  concept unwrap_and_assignable =
    std::is_lvalue_reference_v<traits::unwrap_t<M&>> and
    std::assignable_from<traits::unwrap_t<M&>, T>;

  /**
  * @brief unit CPOの実装
  */
  struct unit_impl {

    /**
    * @brief unwrapした結果に代入する
    */
    template<unwrappable M, typename T>
      requires unwrap_and_assignable<M, T>
    constexpr void operator()(M& m, T&& t) const noexcept(noexcept(cpo::unwrap(m) = std::forward<T>(t))) {
      cpo::unwrap(m) = std::forward<T>(t);
    }

    /**
    * @brief モナド的型のオブジェクトそのものに直接代入する
    */
    template<unwrappable M, typename T>
      requires (not unwrap_and_assignable<M, T>) and
               std::assignable_from<M&, T>
    constexpr void operator()(M& m, T&& t) const noexcept(noexcept(m = std::forward<T>(t))) {
      m = std::forward<T>(t);
    }
  };

}

namespace harmony::inline cpo {

  /**
  * @brief unit(a, r)のように呼び出し、rをモナドaへ再代入する（unitあるいはreturnと呼ばれる操作に対応する）
  */
  inline constexpr detail::unit_impl unit{};
}

namespace harmony::inline concepts {

  /**
  * @brief モナド的な型MはTの値を再代入可能である
  */
  template<typename M, typename T>
  concept rewrappable = 
    unwrappable<M> and
    requires(M& m, T&& v) {
      harmony::cpo::unit(m, std::forward<T>(v));
    };

  /**
  * @brief モナド的な型Mに対するCallbleなFの呼び出し結果は、Mに再代入可能である（一般にbindと呼ばれる操作が可能な組である）
  */
  template<typename F, typename M>
  concept monadic = 
    std::invocable<F, traits::unwrap_t<M>> and
    rewrappable<M, std::invoke_result_t<F, traits::unwrap_t<M>>>;

}

namespace harmony::detail {

  template<typename T>
  concept pointer_like =
    std::is_pointer_v<T> or
    (weakly_indirectly_readable<T> and
    requires(T& t) {
      bool(t);
      t = nullptr;
      T(nullptr);
    });

  template<typename T>
  concept error_func_usable = requires(T&& t) {
    {std::forward<T>(t).error()} -> not_void;
  };

  template<typename T>
  concept unwrap_err_func_usable = requires(T&& t) {
    {std::forward<T>(t).unwrap_err()} -> not_void;
  };

  /**
  * @brief unwrap_other CPOの実装
  * @details 事前条件 : それぞれの処理は有効値を保持していない事を仮定する（あらかじめvalidate()してから利用する）
  */
  struct unwrap_other_impl {

    /**
    * @brief std::optionalの無効値はstd::nullopt
    */
    template<typename T>
    constexpr auto operator()(const std::optional<T>&) const noexcept -> std::nullopt_t {
      return std::nullopt;
    }

    /**
    * @brief pointer的な型の無効値はnullptr
    */
    template<maybe T>
      requires pointer_like<T>
    constexpr auto operator()(const T&) const noexcept -> std::nullptr_t {
      return nullptr;
    }

    /**
    * @brief error()メンバ関数によって無効値を取得
    */
    template<maybe T>
      requires error_func_usable<T>
    constexpr decltype(auto) operator()(T&& t) const noexcept {
      return std::forward<T>(t).error();
    }

    /**
    * @brief unwrap_err()メンバ関数によって無効値を取得
    */
    template<maybe T>
      requires unwrap_err_func_usable<T>
    constexpr decltype(auto) operator()(T&& t) const noexcept {
      return std::forward<T>(t).unwrap_err();
    }

    /**
    * @brief 2要素variantは1つ目の型の値を取得
    */
    template<variant_like V>
    [[nodiscard]]
    constexpr decltype(auto) operator()(V&& v) const noexcept {
      using std::get;
      return get<0>(std::forward<V>(v));
    }
  };

} // namespace harmony::detail

namespace harmony::inline cpo {

  /**
  * @brief unwrap_other(a)のように呼び出し、任意の型の内包するもう一方の値を取り出すカスタマイゼーションポイントオブジェクト
  */
  inline constexpr detail::unwrap_other_impl unwrap_other{};
}

namespace harmony::inline concepts {

  /**
  * @brief Eitherモナドである
  */
  template<typename T>
  concept either = 
    maybe<T> and
    requires(T&& t) {
      {cpo::unwrap_other(std::forward<T>(t))} -> not_void;
    };
} // namespace harmony::inline concepts

namespace harmony::traits {

  /**
  * @brief unwrap_other(a)の結果型を得る
  */
  template<typename T>
  using unwrap_other_t = decltype(harmony::cpo::unwrap_other(std::declval<T>()));
}

namespace harmony {

  namespace detail {
    template<typename M, typename F, typename R = std::invoke_result_t<F, traits::unwrap_t<M&>>>
    inline constexpr bool monadic_noexecpt_v = noexcept(cpo::unit(std::declval<M&>(), std::declval<R>()));

    template<typename F, typename T>
    concept map_reusable = requires(T&& t, F&& f) {
      { std::forward<T>(t).map(std::forward<F>(f))} -> unwrappable;
    };

    template<typename F, typename T>
    concept map_err_reusable = requires(T&& t, F&& f) {
      { std::forward<T>(t).map_err(std::forward<F>(f)) } -> either;
    };

    template<typename F, typename T>
    concept map_error_reusable = requires(T&& t, F&& f) {
      { std::forward<T>(t).map_error(std::forward<F>(f)) } -> either;
    };

    template<typename F, typename T>
    concept and_then_reusable = requires(T&& t, F&& f) {
      { std::forward<T>(t).and_then(std::forward<F>(f))} -> either;
    };

    template<typename F, typename T>
    concept or_else_reusable = requires(T&& t, F&& f) {
      { std::forward<T>(t).or_else(std::forward<F>(f))} -> either;
    };

    template<typename V, typename T>
    concept value_or_reusable = requires(T&& t, V&& v) {
      { std::forward<T>(t).value_or(std::forward<V>(v))} -> std::same_as<V>;
    };
    

    template<typename F, typename T>
    concept not_map_reusable = not map_reusable<F, T>;

    template<typename F, typename T>
    concept not_map_err_reusable = not map_err_reusable<F, T>;

    template<typename F, typename T>
    concept not_map_error_reusable = not map_error_reusable<F, T>;

    template<typename F, typename T>
    concept not_and_then_reusable = not and_then_reusable<F, T>;

    template<typename F, typename T>
    concept not_or_else_reusable = not or_else_reusable<F, T>;
  }

  /**
  * @brief モナド的な型をラップし統一的かつ透過的に扱いつつ、operator|によるbindサポートを追加する
  * @tparam T 任意のモナド的な型、右辺値参照ではないこと
  * @details 基本的には型を明示的に指定せずに、クラステンプレートの引数推論に任せて利用する（それを前提とした振る舞いをする）
  */
  template<unwrappable T>
  class monas {

    static_assert(not std::is_rvalue_reference_v<T>, "T must not be a rvalue reference type.");
    
    // lvalueから初期化された際、Tは左辺値参照となる
    static constexpr bool has_reference = std::is_lvalue_reference_v<T>;

    // Tから参照を外した型、lvalueから初期化された時だけTと異なる
    using M = std::remove_reference_t<T>;

    // lvalueから初期化された場合はその参照を、xvalueから初期化された場合はmoveしてオブジェクトを保持
    T m_monad;
    
  public:

    /**
    * @brief Tが参照である場合のコンストラクタ
    */
    constexpr monas(T bound) noexcept requires has_reference
      : m_monad(bound) {}

    /**
    * @brief Tがprvalueである場合のコンストラクタ
    * @details 実際の引数の型UとTは異なる場合がある（const/volatileを推論補助で外しているため）
    */
    template<typename U>
      requires std::constructible_from<T, U>
    constexpr monas(U&& bound) noexcept(std::is_nothrow_constructible_v<T, U>) requires (not has_reference)
      : m_monad(std::forward<U>(bound)) {}
    
    /**
    * @brief future-likeな型から構築された時のコンストラクタ
    */
    template<concepts::future_like U>
    constexpr monas(U&& future)
      : m_monad{}
    {
      m_monad = cpo::unwrap(std::forward<U>(future));
    }

    /**
    * @brief 保持するモナド的オブジェクトの有効値を取得
    */
    [[nodiscard]]
    constexpr decltype(auto) operator*() noexcept(noexcept(cpo::unwrap(m_monad))) {
      return cpo::unwrap(m_monad);
    }

    /**
    * @brief 保持するモナド的オブジェクトの有効性を取得
    */
    [[nodiscard]]
    constexpr explicit operator bool() const noexcept(noexcept(cpo::validate(m_monad))) requires maybe<M> {
      return cpo::validate(m_monad);
    }

    /**
    * @brief 保持するモナド的オブジェクトの無効値を取得
    */
    [[nodiscard]]
    constexpr decltype(auto) unwrap_err() noexcept(noexcept(cpo::unwrap_other(m_monad))) requires either<M> {
      return cpo::unwrap_other(m_monad);
    }

    /**
    * @brief 保持するモナド的オブジェクトの有効値への暗黙変換
    * @details 左辺値参照で保持しているときのオーバーロード、常に参照で返す
    */
    constexpr operator T&() noexcept requires has_reference {
      return m_monad;
    }
    
    /**
    * @brief 保持するモナド的オブジェクトの有効値への暗黙変換
    * @details 直接保持しているときのオーバーロード、参照で返す
    */
    constexpr operator T&() & noexcept requires (not has_reference) {
      return m_monad;
    }

    /**
    * @brief 保持するモナド的オブジェクトの有効値への暗黙変換
    * @details 直接保持しているときのオーバーロード、右辺値参照で返す
    */
    constexpr operator T&&() && noexcept requires (not has_reference) {
      return std::move(m_monad);
    }

    /**
    * @brief 保持するモナド的型がmap関数を利用可能であるならば有効化する
    */
    template<detail::map_reusable<M> F>
    constexpr auto map(F&& f) && noexcept(noexcept(std::move(m_monad).map(std::forward<F>(f)))) {
      return std::move(m_monad).map(std::forward<F>(f));
    }

    /**
    * @brief 保持するモナド的型がmap_err関数を利用可能であるならば有効化する
    */
    template<detail::map_err_reusable<M> F>
    constexpr auto map_err(F&& f) && noexcept(noexcept(std::move(m_monad).map_err(std::forward<F>(f)))) requires either<M> {
      return std::move(m_monad).map_err(std::forward<F>(f));
    }

    /**
    * @brief 保持するモナド的型がmap_error関数を利用可能であるならば有効化する
    */
    template<detail::map_error_reusable<M> F>
    constexpr auto map_err(F&& f) && noexcept(noexcept(std::move(m_monad).map_error(std::forward<F>(f)))) requires either<M> {
      return std::move(m_monad).map_error(std::forward<F>(f));
    }

    /**
    * @brief 保持するモナド的型がand_then関数を利用可能であるならば有効化する
    */
    template<detail::and_then_reusable<M> F>
    constexpr auto and_then(F&& f) && noexcept(noexcept(std::move(m_monad).and_then(std::forward<F>(f)))) requires either<M> {
      return std::move(m_monad).and_then(std::forward<F>(f));
    }

    /**
    * @brief 保持するモナド的型がor_else関数を利用可能であるならば有効化する
    */
    template<detail::or_else_reusable<M> F>
    constexpr auto or_else(F&& f) && noexcept(noexcept(std::move(m_monad).or_else(std::forward<F>(f)))) requires either<M> {
      return std::move(m_monad).or_else(std::forward<F>(f));
    }

    /**
    * @brief 保持するモナド的型がvalue_or関数を利用可能であるならば有効化する
    */
    template<detail::value_or_reusable<M> U>
    constexpr auto value_or(U&& v) && noexcept(noexcept(std::move(m_monad).value_or(std::forward<U>(v)))) {
      return std::move(m_monad).value_or(std::forward<U>(v));
    }

  public:

    /**
    * @brief bind演算子、単にunwrappableな型に対してのもの
    * @param self monas<T>のrvalue
    * @param f Callableオブジェクト
    */
    template<monadic<M> F>
    friend constexpr auto operator|(monas&& self, F&& f) noexcept(detail::monadic_noexecpt_v<T, F>) -> monas<T>&& {
      cpo::unit(self.m_monad, f(*self));
      return std::move(self);
    }

    /**
    * @brief bind演算子、maybeな型に対してのもの
    * @param self monas<T>のrvalue
    * @param f Callableオブジェクト
    */
    template<monadic<M> F>
      requires maybe<M>
    friend constexpr auto operator|(monas&& self, F&& f) noexcept(noexcept(bool(self)) and detail::monadic_noexecpt_v<T, F>) -> monas<T>&& {
      if (self) {
        cpo::unit(self.m_monad, f(*self));
      }
      return std::move(self);
    }

    /**
    * @brief bind演算子、listな型に対してのもの
    * @details リストの各要素に対して渡された関数を呼び出し、再代入する
    * @param self monas<T>のrvalue
    * @param f Callableオブジェクト
    */
    //template<std::invocable<std::ranges::range_reference_t<T>> F>
    template<typename F>
      requires list<M>
    friend constexpr auto operator|(monas&& self, F&& f) noexcept(detail::monadic_noexecpt_v<std::ranges::iterator_t<T>, F>) -> monas<T>&& requires monadic<F, std::ranges::iterator_t<T>> {
      auto it = std::ranges::begin(*self);
      const auto fin = std::ranges::end(*self);

      for (; it != fin; ++it) {
        cpo::unit(it, f(*it));
      }

      return std::move(self);
    }

    /**
    * @brief bind演算子、戻り値を返さないfに対応する
    * @details 保持するunwrappableオブジェクトの中身を渡して呼び出し、元のオブジェクトをそのまま返す
    * @param self monas<T>のrvalue
    * @param f 戻り値なしのCallableオブジェクト
    */
    template<std::invocable<traits::unwrap_t<T>> F>
      requires std::same_as<std::invoke_result_t<F, traits::unwrap_t<T>>, void>
    friend constexpr auto operator|(monas&& self, F&& f) noexcept(std::is_nothrow_invocable_v<F, traits::unwrap_t<T>>) -> monas<T>&& {
      f(*self);
      return std::move(self);
    }

    /**
    * @brief bind演算子、maybeな型に対してのもの、戻り値を返さないfに対応する
    * @details 保持するmaybeオブジェクトの中身を渡して呼び出し、元のオブジェクトをそのまま返す
    * @param self monas<T>のrvalue
    * @param f Callableオブジェクト
    */
    template<std::invocable<traits::unwrap_t<T>> F>
      requires std::same_as<std::invoke_result_t<F, traits::unwrap_t<T>>, void> and
               maybe<M>
    friend constexpr auto operator|(monas&& self, F&& f) noexcept(noexcept(bool(self)) and std::is_nothrow_invocable_v<F, traits::unwrap_t<T>>) -> monas<T>&& {
      if (self) {
        f(*self);
      }
      return std::move(self);
    }
/*
    template<typename F>
      requires std::same_as<std::invoke_result_t<F, std::ranges::range_reference_t<T>>, void> and
               list<M>
    friend constexpr auto operator|(monas&& self, F&& f) noexcept(detail::monadic_noexecpt_v<std::ranges::iterator_t<T>, F>) -> monas<T>&& requires std::invocable<F, std::ranges::range_reference_t<T>> {
      auto it = std::ranges::begin(*self);
      const auto fin = std::ranges::end(*self);

      for (; it != fin; ++it) {
        cpo::unit(it, f(*it));
      }

      return std::move(self);
    }*/

    /**
    * @brief 左辺値monasをxvalueにする
    * @details autoで受けるなどして左辺値に落とした場合に、再びbindを使用する場合に使う
    * @param self monas<T>のrvalue
    */
    [[nodiscard]]
    friend constexpr auto operator~(monas& self) noexcept -> monas<T>&& {
      return std::move(self);
    }
  };

  /**
  * @brief 参照から初期化された時はそのまま
  */
  template<typename T>
    requires (not concepts::future_like<T> and std::is_lvalue_reference_v<T>)
  monas(T&) -> monas<T&>; // &要らないけど可読性のためにあえて書いてる

  /**
  * @brief xvalueから初期化された時はconst(volatileも？)を外す
  */
  template<typename T>
  monas(T&&) -> monas<std::remove_cv_t<T>>;

  /**
  * @brief future-likeな型は特別扱いする
  */
  template<concepts::future_like T>
  monas(T&&) -> monas<traits::unwrap_t<T>>;

  /**
  * @brief Eitherとなる単純なラッパー
  * @tparam R 有効値の値
  * @tparam L 無効値の値
  */
  template<typename L, typename R>
  struct sachet {
    std::variant<L, R> value;
  
    [[nodiscard]]
    constexpr auto operator*() & noexcept -> R& {
      return std::get<1>(value);
    }

    [[nodiscard]]
    constexpr auto operator*() && noexcept -> R&& {
      return std::get<1>(std::move(value));
    }

    [[nodiscard]]
    constexpr operator bool() const noexcept {
      return value.index() == 1ull;
    }

    [[nodiscard]]
    constexpr auto unwrap_err() & noexcept -> L& {
      return std::get<0>(value);
    }

    [[nodiscard]]
    constexpr auto unwrap_err() && noexcept -> L&& {
      return std::get<0>(std::move(value));
    }
    
  };

  /**
  * @brief 2引数をとるクラステンプレートをあえて1引数で使いたい場合のダミーとして嵌め込むタグ型
  */
  struct nil{};

  /**
  * @brief unwrappableな単純なラッパー
  * @tparam T 保持する値の型
  */
  template<typename T>
  struct sachet<nil, T> {
    T value;

    [[nodiscard]]
    constexpr auto operator*() & noexcept -> T& {
      return value;
    }

    [[nodiscard]]
    constexpr auto operator*() && noexcept -> T&& {
      return std::move(value);
    }
  };

  // remove_cvrefしていいのだろうか・・・？
  template<typename T>
  sachet(T&&) -> sachet<nil, std::remove_cvref_t<T>>;

} // namespace harmony

namespace harmony::inline concepts {

  namespace impl {

    template<typename T, template<typename> typename S>
    inline constexpr bool is_specialization_of = false;

    template<typename T, template<typename> typename S>
    inline constexpr bool is_specialization_of<S<T>, S> = true;
  }

  template<typename T, template<typename> typename S>
  concept specialization_of = impl::is_specialization_of<T, S>;
  
  template<typename F, typename Arg>
  concept not_void_resulted = 
    std::invocable<F, Arg> and
    not std::same_as<void, std::invoke_result_t<F, Arg>>;
}

namespace harmony::detail {

  template<typename F>
  struct then_impl {
    [[no_unique_address]] F fmap;

    template<unwrappable M>
      requires requires(M&& m, F& f) { monas(std::forward<M>(m)) | f; }
    friend constexpr specialization_of<monas> auto operator|(M&& m, then_impl self) noexcept(noexcept(monas(std::forward<M>(m)) | self.fmap)) {
      return monas(std::forward<M>(m)) | self.fmap;
    }

    template<typename T>
      requires requires(monas<T>&& m, F& f) {  std::move(m) | f; }
    friend constexpr specialization_of<monas> auto operator|(monas<T>&& m, then_impl self) noexcept(noexcept(std::move(m) | self.fmap)) {
      return std::move(m) | self.fmap;
    }

  };

  template<typename F>
  then_impl(F&&) -> then_impl<F>;

}

namespace harmony::inline monadic_op{

  /**
  * @brief 左辺のオブジェクトをmonasで包んだうえで、右辺のCallableをbindする
  * @brief チェーンの最初のオブジェクトをmonasで包むのを短縮する
  * @param f bindするCallableオブジェクト
  */
  inline constexpr auto then = []<typename F>(F&& f) noexcept(std::is_nothrow_move_constructible_v<F>) -> detail::then_impl<F> {
    return detail::then_impl{.fmap = std::forward<F>(f) };
  };
}

namespace harmony::detail {

  template<typename F, typename M, typename R>
  consteval bool check_nothrow_map() {
    bool common = noexcept(cpo::unwrap(std::declval<M>())) and std::is_nothrow_invocable_v<F, traits::unwrap_t<M>>;
    if constexpr (unwrappable<R>) {
      return common and std::is_nothrow_constructible_v<monas<R>, R>;
    } else {
      return common and std::is_nothrow_constructible_v<monas<sachet<nil, std::remove_cvref_t<R>>>, R>;
    }
  }
  
  template<typename From, typename To>
  inline constexpr bool is_ptr_to_opt_v = false;
  
  template<typename T>
  inline constexpr bool is_ptr_to_opt_v<std::nullptr_t, std::optional<T>> = true;


  template<typename F>
  struct map_impl {

    [[no_unique_address]] F fmap;

    template<unwrappable M>
      requires detail::map_reusable<F&, M> and
               not_void_resulted<F, traits::unwrap_t<M>>
    friend constexpr specialization_of<monas> auto operator|(M&& m, map_impl self) noexcept(noexcept(std::forward<M>(m).map(self.fmap))) {
      return monas(std::forward<M>(m).map(self.fmap));
    }

    template<bool monadic_return, typename T>
    constexpr auto invoke_impl(T&& v) {
      if constexpr (monadic_return) {
        // map結果がモナド的な型の値ならば、単にmonasで包んで返す
        return monas(this->fmap(std::forward<T>(v)));
      } else {
        // map結果はモナド的な型ではない時、sachetで包んでmonasで包んで返す
        return monas(sachet{ .value = this->fmap(std::forward<T>(v)) });
      }
    }

    template<unwrappable M>
      requires not_void_resulted<F, traits::unwrap_t<M>>
    friend constexpr specialization_of<monas> auto operator|(M&& m, map_impl self) noexcept(check_nothrow_map<F, M, std::invoke_result_t<F, traits::unwrap_t<M>>>()) {
      return self.invoke_impl<unwrappable<std::invoke_result_t<F, traits::unwrap_t<M>>>>(cpo::unwrap(std::forward<M>(m)));
    }

    template<either M>
      requires detail::not_map_reusable<F&, M> and
               not_void_resulted<F, traits::unwrap_t<M>> and
               either<std::remove_reference_t<std::invoke_result_t<F, traits::unwrap_t<M>>>>
    friend constexpr specialization_of<monas> auto operator|(M&& m, map_impl self) {
      using result_t = std::remove_cv_t<std::invoke_result_t<F, traits::unwrap_t<M>>>;

      if (cpo::validate(m)) {
        return monas<result_t>(self.fmap(cpo::unwrap(std::forward<M>(m))));
      } else {
        // 無効値の変換処理
        if constexpr (is_ptr_to_opt_v<std::remove_cvref_t<traits::unwrap_other_t<M>>, result_t>) {
          // nullptr -> nulloptへの無効値の変換（利便性のための特殊対応）
          return monas<result_t>(std::nullopt);
        } else {
          static_assert(std::constructible_from<result_t, traits::unwrap_other_t<M>>, "Cannot convert left value type");
          // その他デフォルト、そのまま構築を試みる
          return monas<result_t>(cpo::unwrap_other(std::forward<M>(m)));
        }
      }
    }
    
    template<either M>
      requires detail::not_map_reusable<F&, M> and
               not_void_resulted<F, traits::unwrap_t<M>>
    friend constexpr specialization_of<monas> auto operator|(M&& m, map_impl self) {
      // 有効値の型
      using R = std::remove_cvref_t<std::invoke_result_t<F, traits::unwrap_t<M>>>;
      // 無効値の型
      using L = std::remove_cvref_t<traits::unwrap_other_t<M>>;
      
      if (cpo::validate(m)) {
        return monas(sachet<L, R>{ .value = std::variant<L, R>(std::in_place_index<1>, self.fmap(cpo::unwrap(std::forward<M>(m)))) });
      } else {
        return monas(sachet<L, R>{ .value = std::variant<L, R>(std::in_place_index<0>, cpo::unwrap_other(std::forward<M>(m))) });
      }
    }
  };

  template<typename F>
  map_impl(F&&) -> map_impl<F>;

} // namespace harmony::detail

namespace harmony::inline monadic_op {

  /**
  * @brief モナド的な型に対してmapを適用する（有効値を任意の型の値へ写す）
  * @brief M<T, E>のような型をM<U, E>のように変換する
  * @param f T -> U へmapするCallableオブジェクト
  */
  inline constexpr auto map = []<typename F>(F&& f) noexcept(std::is_nothrow_move_constructible_v<F>) -> detail::map_impl<F> {
    return detail::map_impl{ .fmap = std::forward<F>(f) };
  };

  inline constexpr auto& transform = map;

} // namespace harmony::inline monadic_op


namespace harmony::detail {

  template<typename F, typename M>
  constexpr bool check_nothrow_reuse_map_err() {
    if constexpr (detail::map_err_reusable<F&, M>) {
      return noexcept(std::declval<M>().map_err(std::declval<F>()));
    } else {
      return noexcept(std::declval<M>().map_error(std::declval<F>()));
    }
  }

  template<typename F, typename M>
  concept map_err_func_reusable = map_err_reusable<F, M> or map_error_reusable<F, M>;

  template<typename F, typename M>
  concept not_map_err_func_reusable = not map_err_func_reusable<F, M>;


  template<typename F>
  struct map_err_impl {

    [[no_unique_address]] F fmap;

    template<either M>
      requires map_err_func_reusable<F&, M> and
               not_void_resulted<F, traits::unwrap_other_t<M>>
    friend constexpr specialization_of<monas> auto operator|(M&& m, map_err_impl self) noexcept(check_nothrow_reuse_map_err<F&, M>()) {
      if constexpr (map_err_reusable<F&, M>) {
        return monas(std::forward<M>(m).map_err(self.fmap));
      } else {
        return monas(std::forward<M>(m).map_error(self.fmap));
      }
    }

    template<either M>
      requires not_map_err_func_reusable<F&, M> and
               not_void_resulted<F, traits::unwrap_other_t<M>> and
               either<std::remove_reference_t<std::invoke_result_t<F, traits::unwrap_other_t<M>>>>
    friend constexpr specialization_of<monas> auto operator|(M&& m, map_err_impl self) {
      using result_t = std::remove_cv_t<std::invoke_result_t<F, traits::unwrap_other_t<M>>>;

      if (cpo::validate(m)) {
        static_assert(std::constructible_from<result_t, traits::unwrap_t<M>>, "Cannot convert right value type");
        // そのまま構築を試みる
        return monas<result_t>(cpo::unwrap(std::forward<M>(m)));
      } else {
        return monas<result_t>(self.fmap(cpo::unwrap_other(std::forward<M>(m))));
      }
    }
    
    template<either M>
      requires not_map_err_func_reusable<F&, M> and
               not_void_resulted<F, traits::unwrap_other_t<M>>
    friend constexpr specialization_of<monas> auto operator|(M&& m, map_err_impl self) {
      // 有効値の型
      using R = std::remove_cvref_t<traits::unwrap_t<M>>;
      // 無効値の型
      using L = std::remove_cvref_t<std::invoke_result_t<F, traits::unwrap_other_t<M>>>;
      
      if (cpo::validate(m)) {
        return monas(sachet<L, R>{ .value = std::variant<L, R>(std::in_place_index<1>, cpo::unwrap(std::forward<M>(m))) });
      } else {
        return monas(sachet<L, R>{ .value = std::variant<L, R>(std::in_place_index<0>, self.fmap(cpo::unwrap_other(std::forward<M>(m)))) });
      }
    }

  };

  template<typename F>
  map_err_impl(F&&) -> map_err_impl<F>;

} // namespace harmony::detail

namespace harmony::inline monadic_op {

  /**
  * @brief Eitherモナドな型に対してmap_errを適用する（無効地を任意の型の値へ写す）
  * @brief M<T, E>のような型をM<T, F>のように変換する（Either<L, R>をEither<M, R>へ変換する）
  * @param f E -> F へmapするCallableオブジェクト
  */
  inline constexpr auto map_err = []<typename F>(F&& f) noexcept(std::is_nothrow_move_constructible_v<F>) -> detail::map_err_impl<F> {
    return detail::map_err_impl{ .fmap = std::forward<F>(f) };
  };

} // namespace harmony::inline monadic_op


namespace harmony::detail {

  template<typename F, typename M, typename R = std::remove_reference_t<std::invoke_result_t<F, traits::unwrap_t<M>>>>
  inline constexpr bool check_nothrow_and_then_v =
    std::is_nothrow_invocable_v<F, traits::unwrap_t<M>> and
    std::is_nothrow_constructible_v<monas<R>, R> and
    std::is_nothrow_constructible_v<monas<R>, traits::unwrap_other_t<M>>;
  


  template<typename F>
  struct and_then_impl {

    [[no_unique_address]] F fmap;
    
    template<either M>
      requires not_and_then_reusable<F&, M> and
               std::invocable<F, traits::unwrap_t<M>> and
               either<std::invoke_result_t<F, traits::unwrap_t<M>>> and
               std::constructible_from<std::remove_reference_t<std::invoke_result_t<F, traits::unwrap_t<M>>>, traits::unwrap_other_t<M>>
    friend constexpr specialization_of<monas> auto operator|(M&& m, and_then_impl self) noexcept(noexcept(cpo::validate(m)) and check_nothrow_and_then_v<F, M>) {
      // 呼び出し結果が左辺値参照を返すとき、コピーされることになる
      // mが有効値を保持していないとき、戻り値のmonasはmの無効値をムーブするしかない（参照するのは危険）
      //using ret_either_t = std::remove_reference_t<decltype(self.fmap(cpo::unwrap(std::forward<M>(m))))>;
      using result_t = std::remove_cvref_t<std::invoke_result_t<F&, traits::unwrap_t<M>>>;

      if (cpo::validate(m)) {
        return monas<result_t>(self.fmap(cpo::unwrap(std::forward<M>(m))));
      } else {
        static_assert(std::constructible_from<result_t, traits::unwrap_other_t<M>>, "Cannot convert left value type");
        return monas<result_t>(cpo::unwrap_other(std::forward<M>(m)));
      }
    }

    template<either M>
      requires and_then_reusable<F&, M>
    friend constexpr specialization_of<monas> auto operator|(M&& m, and_then_impl self) noexcept(noexcept(monas(std::forward<M>(m).and_then(self.fmap)))) {
      return monas(std::forward<M>(m).and_then(self.fmap));
    }
  };
  
  template<typename F>
  and_then_impl(F&&) -> and_then_impl<F>;
}

namespace harmony::inline monadic_op {

  /**
  * @brief Eitherモナドな型の有効値をmapした別のEither型へ変換する（有効値だけを変換し、無効値はそのまま）
  * @brief M<T, E>のような型をN<U, E>のように変換する（Either1<L, R>をEither2<L, S>へ変換する）
  * @param f T -> N<U, E> へmapするCallableオブジェクト（戻り値型はEitherを返さなければならない）
  */
  inline constexpr auto and_then = []<typename F>(F&& f) noexcept(std::is_nothrow_move_constructible_v<F>) -> detail::and_then_impl<F> {
    return detail::and_then_impl{ .fmap = std::forward<F>(f) };
  };
}


namespace harmony::detail {

  template<typename F, typename M, typename R = std::remove_reference_t<std::invoke_result_t<F, traits::unwrap_other_t<M>>>>
  inline constexpr bool check_nothrow_or_else_v =
    std::is_nothrow_invocable_v<F, traits::unwrap_other_t<M>> and
    std::is_nothrow_constructible_v<monas<R>, R> and
    std::is_nothrow_constructible_v<monas<R>, traits::unwrap_t<M>>;


  template<typename F>
  struct or_else_impl {

    [[no_unique_address]] F fmap;

    template<either M>
      requires not_or_else_reusable<F&, M> and
               std::invocable<F, traits::unwrap_other_t<M>> and
               either<std::invoke_result_t<F, traits::unwrap_other_t<M>>> and
               std::constructible_from<std::remove_reference_t<std::invoke_result_t<F, traits::unwrap_other_t<M>>>, traits::unwrap_t<M>>
    friend constexpr specialization_of<monas> auto operator|(M&& m, or_else_impl self) noexcept(noexcept(cpo::validate(m)) and check_nothrow_or_else_v<F, M>) {
      // 呼び出し結果が左辺値参照を返すとき、コピーされることになる
      // mが有効値を保持しているとき、戻り値のmonasはmの有効値をムーブするしかない（参照するのは危険）
      //using ret_either_t = std::remove_reference_t<decltype(self.fmap(cpo::unwrap_other(std::forward<M>(m))))>;
      using result_t = std::remove_cvref_t<std::invoke_result_t<F &, traits::unwrap_other_t<M>>>;

      if (cpo::validate(m)) {
        static_assert(std::constructible_from<result_t, traits::unwrap_t<M>>, "Cannot convert right value type");
        return monas<result_t>(cpo::unwrap(std::forward<M>(m)));
      } else {
        return monas<result_t>(self.fmap(cpo::unwrap_other(std::forward<M>(m))));
      }
    }

    template<either M>
      requires or_else_reusable<F&, M>
    friend constexpr specialization_of<monas> auto operator|(M&& m, or_else_impl self) noexcept(noexcept(monas(std::forward<M>(m).or_else(self.fmap)))) {
      return monas(std::forward<M>(m).or_else(self.fmap));
    }
    
  };
  
  template<typename F>
  or_else_impl(F&&) -> or_else_impl<F>;

} // namespace harmony::detail

namespace harmony::inline monadic_op {

  /**
  * @brief Eitherモナドな型の無効値をmapした別のEither型へ変換する（無効値だけを変換し、有効値はそのまま）
  * @brief M<T, E>のような型をN<T, F>のように変換する（Either1<L, R>をEither2<L, S>へ変換する）
  * @param f E -> N<T, F> へmapするCallableオブジェクト（戻り値型はEitherを返さなければならない）
  */
  inline constexpr auto or_else = []<typename F>(F&& f) noexcept(std::is_nothrow_move_constructible_v<F>) -> detail::or_else_impl<F> {
    return detail::or_else_impl{ .fmap = std::forward<F>(f) };
  };
}

namespace harmony::detail {

  template<typename Fok, typename Ferr>
  struct match_impl {
    [[no_unique_address]] Fok  fmap_ok;
    [[no_unique_address]] Ferr fmap_err;

    template<typename R, typename M, typename Fe>
    constexpr auto invoke_impl(M&& m, Fe& ferr) {
      if constexpr (unwrappable<R>) {
        if (cpo::validate(m)) {
          return monas<R>(this->fmap_ok(cpo::unwrap(std::forward<M>(m))));
        } else {
          return monas<R>(ferr(cpo::unwrap_other(std::forward<M>(m))));
        }
      } else {
        // なぜかR=voidの時もこのまま動くらしい（本当にポータブル？）
        if (cpo::validate(m)) {
          return static_cast<R>(this->fmap_ok(cpo::unwrap(std::forward<M>(m))));
        } else {
          return static_cast<R>(ferr(cpo::unwrap_other(std::forward<M>(m))));
        }
      }
    }


    template<either M>
      requires std::invocable<Fok, traits::unwrap_t<M>> and
               std::invocable<Ferr, traits::unwrap_other_t<M>> and
               std::common_with<std::invoke_result_t<Fok, traits::unwrap_t<M>>, std::invoke_result_t<Ferr, traits::unwrap_other_t<M>>>
    friend constexpr auto operator|(M&& m, match_impl self) {
      using result_t = std::common_type_t<std::invoke_result_t<Fok, traits::unwrap_t<M>>, std::invoke_result_t<Ferr, traits::unwrap_other_t<M>>>;

      return self.invoke_impl<result_t>(std::forward<M>(m), self.fmap_err);
    }

    template<either M>
      requires std::same_as<nil, Ferr> and
               std::invocable<Fok, traits::unwrap_t<M>> and
               std::invocable<Fok, traits::unwrap_other_t<M>> and
               std::common_with<std::invoke_result_t<Fok, traits::unwrap_t<M>>, std::invoke_result_t<Fok, traits::unwrap_other_t<M>>>
    friend constexpr auto operator|(M&& m, match_impl self) {
      using result_t = std::common_type_t<std::invoke_result_t<Fok, traits::unwrap_t<M>>, std::invoke_result_t<Fok, traits::unwrap_other_t<M>>>;

      return self.invoke_impl<result_t>(std::forward<M>(m), self.fmap_ok);
    }
  };

  template<typename Fok, typename Ferr>
  match_impl(Fok&&, Ferr&&) -> match_impl<Fok, Ferr>;

} // namespace harmony::detail

namespace harmony::inline monadic_op {

  /**
  * @brief Eitherモナドな型の有効値と無効値をmapする
  * @brief M<T, E>のような型の有効値と無効値をそれぞれ共通の型Rに変換する
  * @param fok T -> R へmapするCallableオブジェクト
  * @param fee E -> R へmapするCallableオブジェクト（省略された場合、どちらに対してもfokによるmapを試みる）
  * @return Rがunwrappableならばmonas<R>、それ以外の場合はRのオブジェクト、R = voidならば戻り値はない
  */
  inline constexpr auto match = []<typename Fok, typename Ferr = nil>(Fok&& fok, Ferr&& ferr = {}) noexcept(std::is_nothrow_move_constructible_v<Fok> and std::is_nothrow_move_constructible_v<Ferr>) -> detail::match_impl<Fok, Ferr> {
    return detail::match_impl{ .fmap_ok = std::forward<Fok>(fok), .fmap_err = std::forward<Ferr>(ferr) };
  };

  inline constexpr auto &fold = match;

} // namespace harmony::inline monadic_op

namespace harmony::detail {

  template<typename Pred>
  struct exists_impl {
    [[no_unique_address]] Pred f_pred;

    template<maybe M>
      requires std::predicate<Pred, traits::unwrap_t<M>>
    friend constexpr bool operator|(M&& m, exists_impl self) noexcept(noexcept(cpo::validate(m)) and noexcept(self.f_pred(cpo::unwrap(m)))) {
      if (cpo::validate(m)) {
        return self.f_pred(cpo::unwrap(m));
      }
      return false;
    }

    template<list M>
      requires std::predicate<Pred, std::ranges::range_reference_t<M>>
    friend constexpr bool operator|(M&& m, exists_impl self) noexcept(noexcept(std::is_nothrow_invocable_r_v<bool, Pred, std::ranges::range_reference_t<M>>)) {
      auto it = std::ranges::begin(m);
      const auto fin = std::ranges::end(m);

      for (; it != fin; ++it) {
        if (self.f_pred(*it)) return true;
      }

      return false;
    }
  };

  template<typename F>
  exists_impl(F&&) -> exists_impl<F>;
}

namespace harmony::inline monadic_op {

  /**
  * @brief maybeモナドな型の有効値が指定された条件を満たすかをチェックする
  * @param f 有効値に対する述語オブジェクト
  * @return 無効値を保持していた場合はfalse、そうでなければfを適用した結果のbool値
  */
  inline constexpr auto exists = []<typename F>(F&& f) noexcept(std::is_nothrow_move_constructible_v<F>) -> detail::exists_impl<F> {
    return detail::exists_impl{ .f_pred = std::forward<F>(f) };
  };

}

namespace harmony::inline monadic_op {

  /**
  * @brief 受け取った関数を呼び出し、その結果か例外ポインタのどちらかを保持したeitherを返す
  * @param f 例外を投げうるCallableオブジェクト
  * @param args fの引数
  * @return f(args...)の戻り値型をRとすると、Either<std::exception_ptr, R>のようなオブジェクト
  */
  inline constexpr  auto try_catch = []<typename F, typename... Args>(F&& f, Args&&... args) noexcept -> monas<sachet<std::exception_ptr, std::invoke_result_t<F, Args...>>> {
    using R = std::invoke_result_t<F, Args...>;
    using either_t = sachet<std::exception_ptr, R>;
    using return_t = monas<either_t>;

    try {
      return return_t(either_t{ .value = std::variant<std::exception_ptr, R>(std::in_place_index<1>, std::invoke(std::forward<F>(f), std::forward<Args>(args)...)) });
    } catch(...) {
      return return_t(either_t{ .value = std::variant<std::exception_ptr, R>(std::in_place_index<0>, std::current_exception()) });
    }
  };

}

namespace harmony::detail {

  template<bool C>
  concept not_fold = (not C);

  template<typename T, bool IsFold = false>
  struct map_to_impl {

    template<unwrappable M>
      requires not_fold<IsFold> and without_narrowing_convertible<traits::unwrap_t<M>, T>
    [[nodiscard]]
    friend constexpr auto operator|(monas<M>&& m, map_to_impl) noexcept(noexcept(T(*std::move(m)))) -> T {
      return T(std::move(*m));
    }

    template<maybe M>
      requires not_fold<IsFold> and without_narrowing_convertible<traits::unwrap_t<M>, T> and
               std::default_initializable<T> 
    [[nodiscard]]
    friend constexpr auto operator|(monas<M>&& m, map_to_impl) noexcept(noexcept(T(*std::move(m))) and std::is_nothrow_default_constructible_v<T>) -> T {
      if (m) {
        return T(std::move(*m));
      } else {
        return T{};
      }
    }

    template<either M>
      requires IsFold and
               without_narrowing_convertible<traits::unwrap_t<M>, T> and
               without_narrowing_convertible<traits::unwrap_other_t<M>, T>
    [[nodiscard]]
    friend constexpr auto operator|(monas<M>&& m, map_to_impl) noexcept(noexcept(T(*std::move(m))) and noexcept(T(std::move(m.unwrap_err())))) -> T {
      if (m) {
        return T(std::move(*m));
      } else {
        return T(std::move(m.unwrap_err()));
      }
    }
  };
}

namespace harmony::inline monadic_op {

  /**
  * @brief 有効値を取り出して変換する、有効値を保持しているかのチェックを行わない
  * @details 縮小変換は禁止
  * @tparam T 変換先の型
  */
  template<typename T>
    requires (not std::is_reference_v<T>)
  inline constexpr detail::map_to_impl<T> map_to;

  /**
  * @brief 有効値及び無効値を取り出して同じ型に変換する
  * @brief maybeな型が無効値を持つ場合、Tをデフォルト構築して返す
  * @details 縮小変換は禁止
  * @tparam T 変換先の型
  */
  template<typename T>
    requires (not std::is_reference_v<T>)
  inline constexpr detail::map_to_impl<T, true> fold_to;

} // namespace harmony::inline monadic_op

namespace harmony::detail {

  template<typename U>
  struct value_or_impl {
    U tmp_hold;

    template<unwrappable M>
      requires std::convertible_to<U, traits::unwrap_t<M>> and
               detail::value_or_reusable<U, M>
    [[nodiscard]]
    friend constexpr auto operator|(monas<M>&& m, value_or_impl&& self) noexcept(noexcept(std::move(m).value_or(std::forward<U>(self.tmp_hold)))) {
      return std::move(m).value_or(std::forward<U>(self.tmp_hold));
    }

    template<unwrappable M>
      requires std::convertible_to<U, traits::unwrap_t<M>>
    [[nodiscard]]
    friend constexpr auto operator|(monas<M>&& m, value_or_impl&& self) noexcept(noexcept(cpo::validate(m)) and noexcept(cpo::unwrap(m)) and std::is_nothrow_convertible_v<U, traits::unwrap_t<M>>) {
      using R = traits::unwrap_t<M>;

      if (cpo::validate(m)) {
        return cpo::unwrap(std::move(m));
      } else {
        return R(std::forward<U>(self.tmp_hold));
      }
    }
  };

  template<typename U>
  value_or_impl(U&&) -> value_or_impl<U>;

} // namespace detail

namespace harmony::inline monadic_op {

  /**
  * @brief 有効値を保持していなければ指定された値を返す
  */
  inline constexpr auto value_or = []<typename T>(T&& v) {
    return detail::value_or_impl{ .tmp_hold = std::forward<T>(v) };
  };

} // namespace harmony::inline monadic_op

namespace harmony {
  
  /**
  * @brief 有効値と無効値の関係を反転させるラッパークラス
  * @details このクラスはeitherとなる
  */
  template<maybe T>
  struct abekobe {

    static_assert(not std::is_rvalue_reference_v<T>, "T must not be a rvalue reference type.");

    // lvalueから初期化された際、Tは左辺値参照となる
    static constexpr bool has_reference = std::is_lvalue_reference_v<T>;

    // Tから参照を外した型、lvalueから初期化された時だけTと異なる
    using M = std::remove_reference_t<T>;

    T bound;

    /**
    * @brief 有効値を取得する
    * @return Tの無効値を返す
    */
    [[nodiscard]]
    constexpr decltype(auto) operator*() noexcept(noexcept(cpo::unwrap_other(bound))) requires either<M> {
      return cpo::unwrap_other(bound);
    }

    [[nodiscard]]
    constexpr decltype(auto) operator*() && noexcept(noexcept(cpo::unwrap_other(std::move(bound)))) requires either<M> and (not has_reference) {
      return cpo::unwrap_other(std::move(bound));
    }

    /**
    * @brief 有効値を取得する
    * @brief Tがmaybeでしかない時は、無効値としてT自身を返す
    */
    [[nodiscard]]
    constexpr auto operator*() noexcept -> T& {
      return bound;
    }

    [[nodiscard]]
    constexpr auto operator*() && noexcept -> T&& requires (not has_reference) {
      return std::move(bound);
    }

    /**
    * @brief 有効値を保持しているかを判定する
    * @return Tが有効値を保持していればfalse, そうでないならtrue
    */
    [[nodiscard]]
    constexpr explicit operator bool() const noexcept {
      return not cpo::validate(bound);
    }

    /**
    * @brief 無効値を取得する
    * @return Tの有効値を返す
    */
    [[nodiscard]]
    constexpr decltype(auto) unwrap_err() noexcept(noexcept(cpo::unwrap(bound))) {
      return cpo::unwrap(bound);
    }

    [[nodiscard]]
    constexpr decltype(auto) unwrap_err() && noexcept(noexcept(cpo::unwrap(std::move(bound)))) requires (not has_reference) {
      return cpo::unwrap(std::move(bound));
    }

  };

  template<maybe T>
    requires std::is_lvalue_reference_v<T>
  abekobe(T&) -> abekobe<T&>;

  template<maybe T>
  abekobe(T&&) -> abekobe<std::remove_cv_t<T>>;


  inline constexpr auto invert = []<maybe M>(M&& m) {
    return monas(abekobe{ .bound = std::forward<M>(m) });
  };
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif // _MSC_VER