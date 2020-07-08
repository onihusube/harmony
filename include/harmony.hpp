#include <concepts>

namespace harmony::detail {

  struct unwrap_impl {
    template<typename T>
    auto operator()(T&& t);
  };
  
  struct is_empty_impl {
    template<typename T>
    bool operator()(T v);
  };
  
  template<typename T>
  using with_reference = T&;
  
  template<typename T>
  concept not_void = requires {
    typename with_reference<T>;
  };
  
} // detail

namespace harmony::inline cpo {
  
  inline constexpr detail::unwrap_impl unwrap{};
  
  inline constexpr detail::is_empty_impl empty{};
  
} // harmony::cpo

namespace harmony::traits {
  
  template<typename T>
  using content_t = std::remove_cv_ref_t<decltype(harmony::cpo::unwrap(declval<T&>()))>;
}

namespace harmony {

  template<typename T>
  concept monad = requires(T& m) {
    { harmony::cpo::unwrap(m) } -> detail::not_void;
    { harmony::cpo::empty(m) } -> std::same_as<bool>;
  };

  template<monad T>
  using content_t = std::remove_cv_ref_t<decltype(harmony::cpo::unwrap(declval<T&>()))>;

  template<typename T>
  class harmonic {
    
    T m_value;
    
  public:
    template<typename U = T>
    harmonic(U&& v) : m_value(std::forward<U>(v)) {}
    
    
  };
  
} // harmony