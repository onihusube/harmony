#include <concepts>
#include <utility>
#include <ranges>


namespace harmony::inline concepts {

  template<typename T>
  using with_reference = T&;
  
  template<typename T>
  concept not_void = requires {
    typename with_reference<T>;
  };

  template<typename T>
  concept weakly_indirectly_readable = requires(const T& t) {
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

    static_assert(not (std::is_lvalue_reference_v<T> or std::is_rvalue_reference_v<T>), "T must not be a reference type.");

    T& m_monad;
    
  public:

    constexpr monas(T& monad) noexcept : m_monad(monad) {}

    [[nodiscard]]
    constexpr auto operator*() noexcept(noexcept(cpo::unwrap(m_monad))) -> traits::unwrap_raw_t<T> {
      return cpo::unwrap(m_monad);
    }

    // [[nodiscard]]
    // constexpr auto operator*() && noexcept(noexcept(std::move(cpo::unwrap(m_monad)))) -> traits::unwrap_t<T> {
    //   return std::move(cpo::unwrap(m_monad));
    // }

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept(noexcept(cpo::validate(m_monad))) requires maybe<T> {
      return cpo::validate(m_monad);
    }

    // constexpr operator T() const & noexcept(std::is_nothrow_copy_constructible_v<T>) {
    //   return m_monad;
    // }

    constexpr operator T&() {
      return m_monad;
    }

    // constexpr operator T() && noexcept(std::is_nothrow_move_constructible_v<T>) {
    //   return std::move(m_monad);
    // }

  public:

    template<monadic<T> F>
      requires (not maybe<T>)
    friend constexpr auto operator|(monas&& self, F&& f) noexcept(std::is_nothrow_invocable_r_v<T, F, traits::unwrap_t<T>>) -> monas<T>&& {
      cpo::unit(self.m_monad, f(*self));
      return std::move(self);
    }
    
    template<monadic<T> F>
      requires maybe<T>
    friend constexpr auto operator|(monas&& self, F&& f) noexcept(std::is_nothrow_invocable_r_v<T, F, traits::unwrap_t<T>>) -> monas<T>&& {
      if (self) {
        cpo::unit(self.m_monad, f(*self));
      }
      return std::move(self);
    }
  };
  
  template<typename T>
  monas(T&) -> monas<std::remove_cvref_t<T>>;
  
} // harmony
