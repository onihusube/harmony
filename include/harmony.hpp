#include <concepts>
#include <utility>
#include <ranges>
#include <cassert>
#include <variant>
#include <optional>


namespace harmony::inline concepts {

  template<typename T>
  using with_reference = T&;
  
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

  /**
  * @brief 縮小変換を起こさずにFrom -> Toへ変換可能
  */
  template<typename From, typename To>
  concept without_narrowing_convertible =
    std::convertible_to<From, To> and
    requires (From&& x) {
      { std::type_identity_t<To[]>{std::forward<From>(x)} } -> std::same_as<To[1]>;
    };

}

namespace harmony::detail {
  
  struct unwrap_impl {

    template<weakly_indirectly_readable T>
    [[nodiscard]]
    constexpr decltype(auto) operator()(T&& t) const noexcept(noexcept(*std::forward<T>(t))) {
      return *std::forward<T>(t);
    }

    template<typename T>
      requires (not weakly_indirectly_readable<T>) and
               requires(T&& t) { {std::forward<T>(t).value()} -> not_void; }
    [[nodiscard]]
    constexpr decltype(auto) operator()(T&& t) const noexcept(noexcept(std::forward<T>(t).value())) {
      return std::forward<T>(t).value();
    }

    template<typename T>
      requires (not weakly_indirectly_readable<T>) and
               (not requires(T&& t) { {std::forward<T>(t).value()} -> not_void; }) and
               requires(T&& t) { {std::forward<T>(t).unwrap()} -> not_void; }
    [[nodiscard]]
    constexpr decltype(auto) operator()(T&& t) const noexcept(noexcept(std::forward<T>(t).unwrap())) {
      return std::forward<T>(t).unwrap();
    }

    template<std::ranges::range R>
      requires (not weakly_indirectly_readable<R>)
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
  using unwrap_t = decltype(harmony::cpo::unwrap(std::declval<T>()));
}

namespace harmony::detail {

  struct unit_impl {

    template<unwrappable M, typename T>
      requires std::is_lvalue_reference_v<traits::unwrap_t<M&>> and
               std::assignable_from<traits::unwrap_t<M&>, T>
    constexpr void operator()(M& m, T&& t) const noexcept(noexcept(cpo::unwrap(m) = std::forward<T>(t))) {
      cpo::unwrap(m) = std::forward<T>(t);
    }

    template<unwrappable M, typename T>
      requires (not (std::is_lvalue_reference_v<traits::unwrap_t<M&>> and std::assignable_from<traits::unwrap_t<M&>, T>)) and
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
    constexpr decltype(auto) operator()(T&& t) const noexcept(noexcept(std::forward<T>(t).error())) {
      return std::forward<T>(t).error();
    }

    template<maybe T>
      requires (not has_error_func<T>) and
               has_unwrap_err_func<T>
    constexpr decltype(auto) operator()(T&& t) const noexcept(noexcept(std::forward<T>(t).unwrap_err())) {
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
  using unwrap_other_t = decltype(harmony::cpo::unwrap_other(std::declval<T>()));
}

namespace harmony {

  namespace detail {
    template<typename M, typename F, typename R = std::invoke_result_t<F, traits::unwrap_t<M&>>>
    inline constexpr bool monadic_noexecpt_v = noexcept(cpo::unit(std::declval<M&>(), std::declval<R>()));

    template<typename F, typename T>
    concept and_then_reusable = requires(T&& t, F&& f) {
      { std::forward<T>(t).and_then(std::forward<F>(f))} -> either;
    };

    template<typename F, typename T>
    concept or_else_reusable = requires(T&& t, F&& f) {
      { std::forward<T>(t).or_else(std::forward<F>(f))} -> either;
    };
  }


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

    template<typename U>
      requires std::constructible_from<T, U>
    constexpr monas(U&& bound) noexcept(std::is_nothrow_constructible_v<T, U>) requires (not has_reference)
      : m_monad(std::forward<U>(bound)) {}

    [[nodiscard]]
    constexpr auto& operator*() noexcept(noexcept(cpo::unwrap(m_monad))) {
      return cpo::unwrap(m_monad);
    }

    [[nodiscard]]
    constexpr explicit operator bool() const noexcept(noexcept(cpo::validate(m_monad))) requires maybe<M> {
      return cpo::validate(m_monad);
    }

    [[nodiscard]]
    constexpr decltype(auto) unwrap_err() noexcept(noexcept(cpo::unwrap_other(m_monad))) requires either<M> {
      return cpo::unwrap_other(m_monad);
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

    template<detail::and_then_reusable<M> F>
    constexpr auto and_then(F&& f) && noexcept(noexcept(std::move(m_monad).and_then(std::forward<F>(f)))) requires either<M> {
      return std::move(m_monad).and_then(std::forward<F>(f));
    }

    template<detail::or_else_reusable<M> F>
    constexpr auto or_else(F&& f) && noexcept(noexcept(std::move(m_monad).or_else(std::forward<F>(f)))) requires either<M> {
      return std::move(m_monad).or_else(std::forward<F>(f));
    }

  public:

    template<monadic<M> F>
    friend constexpr auto operator|(monas&& self, F&& f) noexcept(detail::monadic_noexecpt_v<T, F>) -> monas<T>&& {
      cpo::unit(self.m_monad, f(*self));
      return std::move(self);
    }
    
    template<monadic<M> F>
      requires maybe<M>
    friend constexpr auto operator|(monas&& self, F&& f) noexcept(noexcept(bool(self)) and detail::monadic_noexecpt_v<T, F>) -> monas<T>&& {
      if (self) {
        cpo::unit(self.m_monad, f(*self));
      }
      return std::move(self);
    }

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

    [[nodiscard]]
    friend constexpr auto operator~(monas& self) noexcept -> monas<T>&& {
      return std::move(self);
    }
  };
  
  template<typename T>
  monas(T&&) -> monas<std::remove_cv_t<T>>;

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

  struct nil{};

  /**
  * @brief unwrappableな単純なラッパー
  * @tparam T 保持する値の型
  */
  template<typename T>
  struct sachet<T, nil> {
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
  sachet(T&&) -> sachet<std::remove_cvref_t<T>, nil>;


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

  template<typename F, typename M, typename R>
  consteval bool check_nothrow_map() {
    bool common = noexcept(cpo::unwrap(std::declval<M>())) and std::is_nothrow_invocable_v<F, traits::unwrap_t<M>>;
    if constexpr (unwrappable<R>) {
      return common and std::is_nothrow_constructible_v<monas<R>, R>;
    } else {
      return common and std::is_nothrow_constructible_v<monas<sachet<std::remove_cvref_t<R>, nil>>, R>;
    }
  }
  
  template<typename From, typename To>
  inline constexpr bool is_ptr_to_opt_v = false;
  
  template<typename T>
  inline constexpr bool is_ptr_to_opt_v<std::nullptr_t, std::optional<T>> = true;


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
    friend constexpr specialization_of<monas> auto operator|(M&& m, map_impl self) noexcept(check_nothrow_map<F, M, std::invoke_result_t<F, traits::unwrap_t<M>>>()) {
      return self.invoke_impl<unwrappable<std::invoke_result_t<F, traits::unwrap_t<M>>>>(cpo::unwrap(std::forward<M>(m)));
    }

    template<either M>
      requires not_void_resulted<F, traits::unwrap_t<M>> and
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
          static_assert(std::constructible_from<traits::unwrap_other_t<M>, result_t>, "Cannot convert left value type");
          // その他デフォルト、そのまま構築を試みる
          return monas<result_t>(cpo::unwrap_other(std::forward<M>(m)));
        }
      }
    }
    
    template<either M>
      requires not_void_resulted<F, traits::unwrap_t<M>> and
               (not either<std::remove_reference_t<std::invoke_result_t<F, traits::unwrap_t<M>>>>)
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

  inline constexpr auto map = []<typename F>(F&& f) noexcept(std::is_nothrow_move_constructible_v<F>) -> detail::map_impl<F> {
    return detail::map_impl{ .fmap = std::forward<F>(f) };
  };

  inline constexpr auto& transform = map;

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
      requires (not and_then_reusable<F, M>) and
               std::invocable<F, traits::unwrap_t<M>> and
               either<std::invoke_result_t<F, traits::unwrap_t<M>>> and
               std::constructible_from<std::remove_reference_t<std::invoke_result_t<F, traits::unwrap_t<M>>>, traits::unwrap_other_t<M>>
    friend constexpr specialization_of<monas> auto operator|(M&& m, and_then_impl self) noexcept(noexcept(cpo::validate(m)) and check_nothrow_and_then_v<F, M>) {
      // 呼び出し結果が左辺値参照を返すとき、コピーされることになる
      // mが有効値を保持していないとき、戻り値のmonasはmの無効値をムーブするしかない（参照するのは危険）
      using ret_either_t = std::remove_reference_t<decltype(self.fmap(cpo::unwrap(std::forward<M>(m))))>;

      if (cpo::validate(m)) {
        return monas<ret_either_t>(self.fmap(cpo::unwrap(std::forward<M>(m))));
      } else {
        return monas<ret_either_t>(cpo::unwrap_other(std::forward<M>(m)));
      }
    }

    template<either M>
      requires and_then_reusable<F, M>
    friend constexpr specialization_of<monas> auto operator|(M&& m, and_then_impl self) noexcept(noexcept(monas(std::forward<M>(m).and_then(self.fmap)))) {
      return monas(std::forward<M>(m).and_then(self.fmap));
    }
  };
  
  template<typename F>
  and_then_impl(F&&) -> and_then_impl<F>;
}

namespace harmony::inline monadic_op {

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
      requires (not or_else_reusable<F, M>) and
               std::invocable<F, traits::unwrap_other_t<M>> and
               either<std::invoke_result_t<F, traits::unwrap_other_t<M>>> and
               std::constructible_from<std::remove_reference_t<std::invoke_result_t<F, traits::unwrap_other_t<M>>>, traits::unwrap_t<M>>
    friend constexpr specialization_of<monas> auto operator|(M&& m, or_else_impl self) noexcept(noexcept(cpo::validate(m)) and check_nothrow_or_else_v<F, M>) {
      // 呼び出し結果が左辺値参照を返すとき、コピーされることになる
      // mが有効値を保持しているとき、戻り値のmonasはmの有効値をムーブするしかない（参照するのは危険）
      using ret_either_t = std::remove_reference_t<decltype(self.fmap(cpo::unwrap_other(std::forward<M>(m))))>;

      if (cpo::validate(m)) {
        return monas<ret_either_t>(cpo::unwrap(std::forward<M>(m)));
      } else {
        return monas<ret_either_t>(self.fmap(cpo::unwrap_other(std::forward<M>(m))));
      }
    }

    template<either M>
      requires or_else_reusable<F, M>
    friend constexpr specialization_of<monas> auto operator|(M&& m, or_else_impl self) noexcept(noexcept(monas(std::forward<M>(m).or_else(self.fmap)))) {
      return monas(std::forward<M>(m).or_else(self.fmap));
    }
    
  };
  
  template<typename F>
  or_else_impl(F&&) -> or_else_impl<F>;

} // namespace harmony::detail

namespace harmony::inline monadic_op {

  inline constexpr auto or_else = []<typename F>(F&& f) noexcept(std::is_nothrow_move_constructible_v<F>) -> detail::or_else_impl<F> {
    return detail::or_else_impl{ .fmap = std::forward<F>(f) };
  };
}

namespace harmony::detail {

  template<typename T>
  struct to_value_impl {

    template<unwrappable M>
      requires without_narrowing_convertible<traits::unwrap_t<M>, T>
    [[nodiscard]]
    friend constexpr auto operator|(monas<M>&& m, to_value_impl) noexcept(noexcept(T(*std::move(m)))) -> T {
      return T(std::move(*m));
    }

    template<maybe M>
      requires std::default_initializable<T> and
               without_narrowing_convertible<traits::unwrap_t<M>, T>
    [[nodiscard]]
    friend constexpr auto operator|(monas<M>&& m, to_value_impl) noexcept(noexcept(T(*std::move(m))) and std::is_nothrow_default_constructible_v<T>) -> T {
      if (m) {
        return T(std::move(*m));
      } else {
        return T();
      }
    }
  };
}

namespace harmony::inline monadic_op {

  template<typename T>
  inline constexpr detail::to_value_impl<T> to_value;

} // namespace harmony::inline monadic_op
