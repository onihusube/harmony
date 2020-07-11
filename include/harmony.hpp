#include <concepts>
#include <utility>
#include <ranges>
#include <optional>


namespace harmony::inline concepts {

  template<typename T>
  using with_reference = T&;
  
  template<typename T>
  concept not_void = requires {
    typename with_reference<T>;
  };

  template<typename T>
  concept weakly_indirectly_readable = requires(T& t) {
    {*t} -> not_void;
  };
}

namespace harmony::detail {
  
  struct unwrap_impl {

    template<typename T>
      requires std::is_pointer_v<T>
    [[nodiscard]]
    constexpr auto& operator()(T t) const noexcept {
      return *t;
    }

    template<weakly_indirectly_readable T>
      requires (not std::is_pointer_v<T>)
    [[nodiscard]]
    constexpr auto& operator()(T& t) const noexcept(noexcept(*t)) {
      return *t;
    }

    template<typename T>
      requires (not weakly_indirectly_readable<T>) and
               requires(T& t) { {t.value()} -> not_void; }
    [[nodiscard]]
    constexpr auto& operator()(T& t) const noexcept(noexcept(t.value())) {
      return t.value();
    }

    template<std::ranges::range R>
    [[nodiscard]]
    constexpr auto operator()(R& t) const noexcept(noexcept(std::ranges::begin(t)) and noexcept(std::ranges::end(t))) {
      struct wrap_view {
        using iterator = std::ranges::iterator_t<R>;

        iterator it;

        [[no_unique_address]]
        std::ranges::sentinel_t<R> se;

        constexpr auto begin() const noexcept {
          return it;
        }

        constexpr auto end() const noexcept {
          return se;
        }
      };

      return wrap_view{ .it = std::ranges::begin(t), .se = std::ranges::end(t)};
    }
  };

} // harmony::detail

namespace harmony::inline cpo {
  
  inline constexpr detail::unwrap_impl unwrap{};
  
}

namespace harmony::inline concepts {

  template<typename T>
  concept unwrappable = requires(T& m) {
    { harmony::cpo::unwrap(m) } -> not_void;
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
    requires(T& m) {
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
      requires std::assignable_from<traits::unwrap_raw_t<M>, T>
    constexpr void operator()(M& m, T&& t) const noexcept(noexcept(cpo::unwrap(m) = std::forward<T>(t))) {
      cpo::unwrap(m) = std::forward<T>(t);
    }

    template<unwrappable M, typename T>
      requires (not std::assignable_from<traits::unwrap_raw_t<M>, T>) and
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
  
  template<typename U, typename T>
  concept equivalent_to = std::same_as<std::remove_cvref_t<U>, T>;

}

namespace harmony {

  template<unwrappable T>
  class monas {

    static_assert(not std::is_rvalue_reference_v<T>, "T must not be a rvalue reference type.");
    
    // lvalueから初期化された際、Tは左辺値参照となる
    static constexpr bool has_reference = std::is_lvalue_reference_v<T>;

    // lvalueから初期化された場合はその参照を、xvalueから初期化された場合はmoveしてオブジェクトを保持
    T m_monad;
    
  public:

    // Tから参照を外した型、lvalueから初期化された時だけTと異なる
    using M = std::remove_reference_t<T>;

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
  
} // harmony

namespace harmony {

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
}

namespace harmony::detail {
  
  template<unwrappable M>
  [[nodiscard]]
  inline constexpr auto suitable_wrap(M&& m) -> monas<M> {
    return monas(std::forward<M>(m));
  }

  template<typename T>
  [[nodiscard]]
  inline constexpr auto suitable_wrap(monas<T>&& m) -> monas<T> {
    return std::move(m);
  }

  template<typename F>
  struct map_impl {

    F fmap;

    template<bool monadic_return, typename M>
    auto invoke_impl(M&& m) {
      if constexpr (monadic_return) {
        // map結果がモナド的な型の値ならば、単にmonasで包んで返す
        return monas(this->fmap(cpo::unwrap(m)));
      } else {
        // map結果はモナド的な型ではない時、sachetで包んでmonasで包んで返す
        return monas(sachet{ .value = this->fmap(cpo::unwrap(m)) });
      }
    }

    template<unwrappable M>
      requires std::invocable<F, traits::unwrap_t<M>>
    [[nodiscard]]
    friend constexpr specialization_of<monas> auto operator|(M&& m, map_impl self) {
      return self.invoke_impl<unwrappable<std::invoke_result_t<F, traits::unwrap_t<M>>>>(std::forward<M>(m));
    }
    
    /*template<maybe M>
      requires std::invocable<F, traits::unwrap_t<M>>
    [[nodiscard]]
    friend constexpr specialization_of<monas> auto operator|(M&& m, map_impl&& self) {
      using ret_t = decltype(self.invoke_impl<unwrappable<std::invoke_result_t<F, traits::unwrap_t<M>>>>(std::forward<M>(m)));

      if (cpo::validate(m)) {
        return self.invoke_impl<unwrappable<std::invoke_result_t<F, traits::unwrap_t<M>>>>(std::forward<M>(m));
      }

      // 無効状態で初期化したい、CPOが必要かも・・・
      return ret_t{typename ret_t::M{}};
    }*/
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
