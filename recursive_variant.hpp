#pragma once

#include <variant>
#include <memory>
#include <type_traits>
#include <utility>

namespace detail
{


template<class T>
class wrapper
{
  public:
    template<class... Args,
             class = std::enable_if_t<
               std::is_constructible_v<T,Args&&...>
             >>
    wrapper(Args&&... args)
      : value_(std::make_unique<T>(std::forward<Args>(args)...))
    {}

    wrapper(wrapper&&) = default;

    wrapper(const wrapper& other)
      : value_(std::make_unique<T>(other.value()))
    {}

    wrapper& operator=(wrapper&&) = default;
    wrapper& operator=(wrapper&) = default;

    T& value()
    {
      return value_;
    }

    const T& value() const
    {
      return value_;
    }

  private:
    std::unique_ptr<T> value_;
};


// see https://stackoverflow.com/a/1956217/722294
namespace 
{


template<class T, int discriminator>
struct is_complete {  
  template<class U, int = sizeof(U)>
  static std::true_type test(int);

  template<class>
  static std::false_type test(...);

  static const bool value = decltype(test<T>(0))::value;
};


} // end anonymous namespace

#define IS_COMPLETE(X) is_complete<X,__COUNTER__>::value


template<class T>
using wrap_if_incomplete_t = std::conditional_t<IS_COMPLETE(T), T, wrapper<T>>;


template<class Function>
struct unwrap_and_call
{
  mutable Function f_;

  // forward unwrapped arguments to f
  template<class Arg>
  auto operator()(Arg&& arg) const
  {
    return f_(std::forward<Arg>(arg));
  }

  // unwrap wrapped arguments before calling f
  template<class Arg>
  auto operator()(wrapper<Arg>& arg) const
  {
    return f_(arg.value());
  }

  template<class Arg>
  auto operator()(const wrapper<Arg>& arg) const
  {
    return f_(arg.value());
  }

  template<class Arg>
  auto operator()(wrapper<Arg>&& arg) const
  {
    return f_(std::move(arg.value()));
  }
};


} // end detail


template<class... Types>
class recursive_variant : public std::variant<detail::wrap_if_incomplete_t<Types>...>
{
  private:
    using super_t = std::variant<detail::wrap_if_incomplete_t<Types>...>;

  public:
    using super_t::super_t;
};


template<class Visitor, class... Types>
constexpr auto visit(Visitor&& visitor, recursive_variant<Types...>& var)
{
  // unwrap wrapped types before the visitor sees them
  detail::unwrap_and_call<std::decay_t<Visitor>> unwrapping_visitor{std::forward<Visitor>(visitor)};

  return std::visit(std::move(unwrapping_visitor), var);
}

template<class Visitor, class... Types>
constexpr auto visit(Visitor&& visitor, const recursive_variant<Types...>& var)
{
  // unwrap wrapped types before the visitor sees them
  detail::unwrap_and_call<std::decay_t<Visitor>> unwrapping_visitor{std::forward<Visitor>(visitor)};

  return std::visit(std::move(unwrapping_visitor), var);
}

template<class Visitor, class... Types>
constexpr auto visit(Visitor&& visitor, recursive_variant<Types...>&& var)
{
  // unwrap wrapped types before the visitor sees them
  detail::unwrap_and_call<std::decay_t<Visitor>> unwrapping_visitor{std::forward<Visitor>(visitor)};

  return std::visit(std::move(unwrapping_visitor), std::move(var));
}


// clean up after ourself
#undef IS_COMPLETE

