#include <concepts>
#include <utility>
#include <ranges>

namespace harmony::detail {
  
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

  struct unwrap_impl {

    template<weakly_indirectly_readable T>
    constexpr auto& operator()(T& t) noexcept(noexcept(*t)) {
      return *t;
    }

    template<typename T>
      requires (not weakly_indirectly_readable<T>) and
               requires(const T& t) { {t.value()} -> not_void; }
    constexpr auto& operator()(T& t) noexcept(noexcept(*t)) {
      return *t;
    }
  };

} // harmony::detail

namespace harmony::inline cpo {
  
  inline constexpr detail::unwrap_impl unwrap{};
  
}

namespace harmony::inline concepts {

  template<typename T>
  concept unwrappable = requires(T& m) {
    { harmony::cpo::unwrap(m) } -> detail::not_void;
  };
}

namespace harmony::detail {

  template<typename T>
  concept boolean_convertible = requires(const T& t) {
    bool(t);
  };

  struct validate_impl {

    template<unwrappable T>
      requires boolean_convertible<T>
    constexpr bool operator()(const T& t) noexcept(noexcept(bool(t))) {
      return bool(t);
    }

    template<unwrappable T>
      requires (not boolean_convertible<T>) and
               requires(const T& t) { {t.has_value()} -> std::same_as<bool>; }
    constexpr bool operator()(const T& t) noexcept(noexcept(t.has_value())) {
      return t.has_value();
    }

    template<unwrappable T>
      requires requires(const T& t) {
        {std::ranges::empty(t)} -> std::same_as<bool>;
      }
    constexpr bool operator()(const T& t) noexcept(noexcept(std::ranges::empty(t))) {
      return not std::ranges::empty(t);
    }
  };
  
} // harmony::detail

namespace harmony::inline cpo {

  inline constexpr detail::validate_impl validate{};
  
}

namespace harmony::inline concepts {

  template<typename T>
  concept maybe = requires(T& m) {
    { harmony::cpo::validate(m) } -> std::same_as<bool>;
  };
  
  template<typename T>
  concept list = maybe<T> and std::ranges::range<T>;
}

namespace harmony::traits {
  
  template<typename T>
  using content_t = std::remove_cvref_t<decltype(harmony::cpo::unwrap(std::declval<T&>()))>;
}

namespace harmony {

  template<unwrappable T>
  class monas {
    
    T m_monad;
    
  public:

    template<std::same_as<T> U>
    constexpr monas(U&& v) : m_monad(std::forward<U>(v)) {}
    
    template<std::invocable<traits::content_t<T>> F>
      requires (not maybe<T>)
    friend constexpr auto operator|(monas& self, F&& f) -> T {
      return f(cpo::unwrap(self.m_monad));
    }
    
    template<std::invocable<traits::content_t<T>> F>
      requires maybe<T>
    friend constexpr auto operator|(monas& self, F&& f) -> T {
      if (cpo::validate(self.m_monad)) {
        return f(cpo::unwrap(self));
      } else {
        return self.m_monad;
      }
    }
    
  };
  
  template<typename T>
  monas(T&&) -> monas<std::remove_cvref_t<T>>;
  
} // harmony
