#pragma once

#include <variant>

namespace Quotient {

//! \brief A minimal subset of std::expected from C++23
template <typename T, typename E,
          std::enable_if_t<!std::is_same_v<T, E>, bool> = true>
class Expected {
private:
    template <typename X>
    using enable_if_constructible_t = std::enable_if_t<
        std::is_constructible_v<T, X> || std::is_constructible_v<E, X>>;

public:
    using value_type = T;
    using error_type = E;

    Expected() = default;
    explicit Expected(const Expected&) = default;
    explicit Expected(Expected&&) noexcept = default;

    template <typename X, typename = enable_if_constructible_t<X>>
    Expected(X&& x)
        : data(std::forward<X>(x))
    {}

    Expected& operator=(const Expected&) = default;
    Expected& operator=(Expected&&) noexcept = default;

    template <typename X, typename = enable_if_constructible_t<X>>
    Expected& operator=(X&& x)
    {
        data = std::forward<X>(x);
        return *this;
    }

    bool has_value() const { return std::holds_alternative<T>(data); }
    explicit operator bool() const { return has_value(); }

    const value_type& value() const& { return std::get<T>(data); }
    value_type& value() & { return std::get<T>(data); }
    value_type value() && { return std::get<T>(std::move(data)); }

    const value_type& operator*() const& { return value(); }
    value_type& operator*() & { return value(); }

    const value_type* operator->() const& { return std::get_if<T>(&data); }
    value_type* operator->() & { return std::get_if<T>(&data); }

    template <class U>
    T value_or(U&& fallback) const&
    {
        if (has_value())
            return value();
        return std::forward<U>(fallback);
    }
    template <class U>
    T value_or(U&& fallback) &&
    {
        if (has_value())
            return value();
        return std::forward<U>(fallback);
    }

    const E& error() const& { return std::get<E>(data); }
    E& error() & { return std::get<E>(data); }

private:
    std::variant<T, E> data;
};

} // namespace Quotient
