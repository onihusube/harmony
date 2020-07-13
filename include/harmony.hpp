#include <concepts>
#include <utility>
#include <ranges>
#include <cassert>
#include <optional>


namespace harmony::inline concepts {

  template<typename T>
  using with_reference = T&;
  
  template<typename T>
  concept not_void = requires {
    typename with_reference<T>;
  };

  template<typename T>
  concept weakly_indirectly_readable = requires(T&& t) {
    {*std::forward<T>(t)} -> not_void;
  };
}

namespace harmony::detail {
  
  struct unwrap_impl {

    template<weakly_indirectly_readable T>
    [[nodiscard]]
    constexpr auto&& operator()(T&& t) const noexcept(noexcept(*std::forward<T>(t))) {
      return *std::forward<T>(t);
    }

    template<typename T>
      requires (not weakly_indirectly_readable<T>) and
               requires(T&& t) { {std::forward<T>(t).value()} -> not_void; }
    [[nodiscard]]
    constexpr auto&& operator()(T&& t) const noexcept(noexcept(std::forward<T>(t).value())) {
      return std::forward<T>(t).value();
    }

    template<typename T>
      requires (not weakly_indirectly_readable<T>) and
               (not requires(T&& t) { {std::forward<T>(t).value()} -> not_void; }) and
               requires(T&& t) { {std::forward<T>(t).unwrap()} -> not_void; }
    [[nodiscard]]
    constexpr auto&& operator()(T&& t) const noexcept(noexcept(std::forward<T>(t).unwrap())) {
      return std::forward<T>(t).unwrap();
    }

    template<std::ranges::range R>
    [[nodiscard]]
    constexpr auto operator()(R&& r) const noexcept -> R&& {
      return std::forward<R>(r);
    }
  };

} // harmony::detail

namespace harmony::inline cpo {
  
  inline constexpr detail::unwrap_impl unwrap{};
  
}

namespace harmony::inline concepts {

  template<typename T>
  concept unwrappable = requires(T&& m) {
    { harmony::cpo::unwrap(std::forward<T>(m)) } -> not_void;
  };

  template<typename T>
  concept boolean_convertible = requires(const T& t) {
    bool(t);
  };
}

namespace harmony::detail {

  struct validate_impl {

    template<unwrappable T>
      requires boolean_convertible<T>
    [[nodiscard]]
    constexpr bool operator()(const T& t) const noexcept(noexcept(bool(t))) {
      return bool(t);
    }

    template<unwrappable T>
      requires (not boolean_convertible<T>) and
               requires(const T& t) { {t.has_value()} -> std::same_as<bool>; }
    [[nodiscard]]
    constexpr bool operator()(const T& t) const noexcept(noexcept(t.has_value())) {
      return t.has_value();
    }

    template<unwrappable T>
      requires (not boolean_convertible<T>) and
               (not requires(const T& t) { {t.has_value()} -> std::same_as<bool>; }) and
               requires(const T& t) { {t.is_ok()} -> std::same_as<bool>; }
    [[nodiscard]]
    constexpr bool operator()(const T& t) const noexcept(noexcept(t.is_ok())) {
      return t.is_ok();
    }

    template<unwrappable T>
      requires (not boolean_convertible<T>) and
               requires(const T& t) { {std::ranges::empty(t)} -> std::same_as<bool>; }
    [[nodiscard]]
    constexpr bool operator()(const T& t) const noexcept(noexcept(std::ranges::empty(t))) {
      return not std::ranges::empty(t);
    }
  };
  
} // harmony::detail

namespace harmony::inline cpo {

  inline constexpr detail::validate_impl validate{};
  
}

namespace harmony::inline concepts {

  template<typename T>
  concept maybe =
    unwrappable<T> and
    requires(const T& m) {
      { harmony::cpo::validate(m) } -> std::same_as<bool>;
    };
  
  template<typename T>
  concept list = maybe<T> and std::ranges::range<T>;

}

namespace harmony::traits {

  template<typename T>
  using unwrap_raw_t = decltype(harmony::cpo::unwrap(std::declval<T&>()));
  
  template<typename T>
  using unwrap_t = std::remove_cvref_t<unwrap_raw_t<T>>;
}

namespace harmony::detail {

  struct unit_impl {

    template<unwrappable M, typename T>
      requires std::is_lvalue_reference_v<traits::unwrap_raw_t<M>> and
               std::assignable_from<traits::unwrap_raw_t<M>, T>
    constexpr void operator()(M& m, T&& t) const noexcept(noexcept(cpo::unwrap(m) = std::forward<T>(t))) {
      cpo::unwrap(m) = std::forward<T>(t);
    }

    template<unwrappable M, typename T>
      requires (not (std::is_lvalue_reference_v<traits::unwrap_raw_t<M>> and std::assignable_from<traits::unwrap_raw_t<M>, T>)) and
               std::assignable_from<M&, T>
    constexpr void operator()(M& m, T&& t) const noexcept(noexcept(m = std::forward<T>(t))) {
      m = std::forward<T>(t);
    }
  };

}

namespace harmony::inline cpo {

  inline constexpr detail::unit_impl unit{};
}

namespace harmony::inline concepts {

  template<typename M, typename T>
  concept rewrappable = 
    unwrappable<M> and
    requires(M& m, T&& v) {
      harmony::cpo::unit(m, std::forward<T>(v));
    };

  template<typename F, typename M>
  concept monadic = 
    std::invocable<F, traits::unwrap_t<M>> and
    rewrappable<M, std::invoke_result_t<F, traits::unwrap_t<M>>>;

}

namespace harmony {

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

    constexpr monas(T& bound) noexcept requires has_reference
      : m_monad(bound) {}

    constexpr monas(T&& bound) noexcept(std::is_nothrow_move_constructible_v<T>) requires (not has_reference)
      : m_monad(std::move(bound)) {}

    [[nodiscard]]
    constexpr auto& operator*() noexcept(noexcept(cpo::unwrap(m_monad))) {
      return cpo::unwrap(m_monad);
    }

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept(noexcept(cpo::validate(m_monad))) requires maybe<M> {
      return cpo::validate(m_monad);
    }

    constexpr operator T&() noexcept requires has_reference {
      return m_monad;
    }
    
    constexpr operator T&() & noexcept requires (not has_reference) {
      return m_monad;
    }

    constexpr operator T&&() && noexcept requires (not has_reference) {
      return std::move(m_monad);
    }

  public:

    template<monadic<M> F>
      requires (not maybe<M>)
    friend constexpr auto operator|(monas&& self, F&& f) noexcept(std::is_nothrow_invocable_r_v<M, F, traits::unwrap_t<M>>) -> monas<T>&& {
      cpo::unit(self.m_monad, f(*self));
      return std::move(self);
    }
    
    template<monadic<M> F>
      requires maybe<M>
    friend constexpr auto operator|(monas&& self, F&& f) noexcept(std::is_nothrow_invocable_r_v<M, F, traits::unwrap_t<M>>) -> monas<T>&& {
      if (self) {
        cpo::unit(self.m_monad, f(*self));
      }
      return std::move(self);
    }

    [[nodiscard]]
    friend constexpr auto operator~(monas& self) noexcept -> monas<T>&& {
      return std::move(self);
    }
  };
  
  template<typename T>
  monas(T&&) -> monas<std::remove_cv_t<T>>;


  template<typename T>
  struct sachet {
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
  
  template<typename T>
  sachet(T&&) -> sachet<std::remove_cvref_t<T>>;
}

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
  struct map_impl {

    [[no_unique_address]] F fmap;

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
    [[nodiscard]]
    friend constexpr specialization_of<monas> auto operator|(M&& m, map_impl self) {
      if constexpr (maybe<M>) {
        // 一応チェックする
        assert(cpo::validate(m));
      }
      return self.invoke_impl<unwrappable<std::invoke_result_t<F, traits::unwrap_t<M>>>>(cpo::unwrap(std::forward<M>(m)));
    }
  };

  template<typename F>
  map_impl(F&&) -> map_impl<F>;

} // namespace harmony::detail

namespace harmony::inline monadic_op {

  inline constexpr auto map = []<typename F>(F&& f) -> detail::map_impl<F> {
    return detail::map_impl{ .fmap = std::forward<F>(f) };
  };

  inline constexpr auto& transform = map;

} // namespace harmony::inline monadic_op

namespace harmony::detail {

  template<typename T>
  concept nullable = requires(T& t) {
    t = nullptr;
  };

  template<typename T>
  concept has_error_func = requires(T&& t) {
    {std::forward<T>(t).error()} -> not_void;
  };

  template<typename T>
  concept has_unwrap_err_func = requires(T&& t) {
    {std::forward<T>(t).unwrap_err()} -> not_void;
  };


  struct unwrap_other_impl {

    template<typename T>
    constexpr auto operator()(const std::optional<T>&) const noexcept -> std::nullopt_t {
      return std::nullopt;
    }

    template<maybe T>
      requires nullable<T>
    constexpr auto operator()(const T&) const noexcept -> std::nullptr_t {
      return nullptr;
    }

    template<maybe T>
      requires has_error_func<T>
    constexpr auto&& operator()(T&& t) const noexcept(noexcept(std::forward<T>(t).error())) {
      return std::forward<T>(t).error();
    }

    template<maybe T>
      requires (not has_error_func<T>) and
               has_unwrap_err_func<T>
    constexpr auto&& operator()(T&& t) const noexcept(noexcept(std::forward<T>(t).unwrap_err())) {
      return std::forward<T>(t).unwrap_err();
    }
  };

} // namespace harmony::detail

namespace harmony::inline cpo {
  inline constexpr detail::unwrap_other_impl unwrap_other{};
}

namespace harmony::inline concepts {

  template<typename T>
  concept either = 
    maybe<T> and
    requires(T&& t) {
      {cpo::unwrap_other(std::forward<T>(t))} -> not_void;
    };
} // namespace harmony::inline concepts

namespace harmony::traits {

  template<typename T>
  using unwrap_other_raw_t = decltype(harmony::cpo::unwrap_other(std::declval<T>()));
  
  template<typename T>
  using unwrap_other_t = std::remove_cvref_t<unwrap_other_raw_t<T>>;
}

namespace harmony::detail {

  template<typename F>
  struct and_then_impl {

    [[no_unique_address]] F fmap;
    
    template<either M>
      requires std::invocable<F, traits::unwrap_t<M>> and
               either<std::invoke_result_t<F, traits::unwrap_t<M>>> and
               std::constructible_from<std::remove_reference_t<std::invoke_result_t<F, traits::unwrap_t<M>>>, traits::unwrap_other_raw_t<M>>
    [[nodiscard]]
    friend constexpr specialization_of<monas> auto operator|(M&& m, and_then_impl self) {
      // 呼び出し結果が左辺値参照を返すとき、コピーされることになる
      // mが有効値を保持していないとき、戻り値のmonasはmの無効値をムーブするしかない（参照するのは危険）
      using ret_either_t = std::remove_reference_t<decltype(self.fmap(cpo::unwrap(std::forward<M>(m))))>;

      if (cpo::validate(m)) {
        return monas<ret_either_t>(self.fmap(cpo::unwrap(std::forward<M>(m))));
      } else {
        return monas<ret_either_t>(cpo::unwrap_other(std::forward<M>(m)));
      }
    }
  };
  
  template<typename F>
  and_then_impl(F&&) -> and_then_impl<F>;
}

namespace harmony::inline monadic_op {

  inline constexpr auto and_then = []<typename F>(F&& f) -> detail::and_then_impl<F> {
    return detail::and_then_impl{ .fmap = std::forward<F>(f) };
  };
}
