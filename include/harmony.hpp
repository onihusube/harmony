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
               requires(const T& t) {
                 {t.value()} -> not_void;
               }
    constexpr auto& operator()(T& t) noexcept(noexcept(*t)) {
      return *t;
    }

    template<typename T>
    auto operator()(T&& t);
  };

  template<typename T>
  concept boolean_convertible = requires(const T& t) {
    bool(t);
  };

  struct is_empty_impl {

    template<boolean_convertible T>
    constexpr bool operator()(const T& t) noexcept(noexcept(bool(t))) {
      return not bool(t);
    }

    template<typename T>
      requires (not boolean_convertible<T>) and
               requires(const T& t) {
                 {t.has_value()} -> std::same_as<bool>;
               }
    constexpr bool operator()(const T& t) noexcept(noexcept(t.has_value())) {
      return not t.has_value();
    }

    template<typename T>
      requires requires(const T& t) {
        {std::ranges::empty(t)} -> std::same_as<bool>;
      }
    constexpr bool operator()(const T& t) noexcept(noexcept(std::ranges::empty(t))) {
      return std::ranges::empty(t);
    }
  };
  
} // detail

namespace harmony::inline cpo {
  
  inline constexpr detail::unwrap_impl unwrap{};
  
  inline constexpr detail::is_empty_impl empty{};
  
} // harmony::cpo

namespace harmony::traits {
  
  template<typename T>
  using content_t = std::remove_cvref_t<decltype(harmony::cpo::unwrap(std::declval<T&>()))>;
}

namespace harmony {

  template<typename T>
  concept monad = requires(T& m) {
    { harmony::cpo::unwrap(m) } -> detail::not_void;
    { harmony::cpo::empty(m) } -> std::same_as<bool>;
  };

  template<typename T>
  class harmonic {
    
    T m_value;
    
  public:
    template<typename U = T>
    harmonic(U&& v) : m_value(std::forward<U>(v)) {}
    
    
  };
  
} // harmony