/******************************************************************************
 * Copyright (C) 2016 Kitsune Ral <kitsune-ral@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include <QtCore/QMetaEnum>
#include <QtCore/QDebug>

#include <functional>

namespace QMatrixClient
{
    /**
     * @brief A crude wrapper around a container of pointers that owns pointers
     * to contained objects
     *
     * Similar to vector<unique_ptr<>>, upon deletion, this wrapper
     * will delete all events contained in it. This wrapper can be used
     * over Qt containers, which are incompatible with unique_ptr and even
     * with QScopedPointer (which is the reason of its creation).
     */
    template <typename ContainerT>
    class Owning : public ContainerT
    {
        public:
            Owning() = default;
            Owning(const Owning&) = delete;
            Owning(Owning&&) = default;
            Owning& operator=(Owning&& other)
            {
                assign(other.release());
                return *this;
            }

            ~Owning() { cleanup(); }

            void assign(ContainerT&& other)
            {
                if (&other == this)
                    return;
                cleanup();
                ContainerT::operator=(other);
            }

            /**
             * @brief returns the underlying container and releases the ownership
             *
             * Acts similar to unique_ptr::release.
             */
            ContainerT release()
            {
                ContainerT c;
                ContainerT::swap(c);
                return c;
            }
        private:
            void cleanup() { for (auto e: *this) delete e; }
    };

    /**
     * @brief Lookup a value by a key in a varargs list
     *
     * This function template takes the value of its first argument (selector)
     * as a key and searches for it in the key-value map passed in
     * a parameter pack (every next pair of arguments forms a key-value pair).
     * If a match is found, the respective value is returned; if no pairs
     * matched, the last value (fallback) is returned.
     *
     * All options should be of the same type or implicitly castable to the
     * type of the first option. If you need some specific type to cast to
     * you can explicitly provide it as the ValueT template parameter
     * (e.g. <code>lookup<void*>(parameters...)</code>). Note that pointers
     * to methods of different classes and even to functions with different
     * signatures are of different types. If their return types are castable
     * to some common one, @see dispatch that deals with this by swallowing
     * the method invocation.
     *
     * Below is an example of usage to select a parser depending on contents of
     * a JSON object:
     * {@code
     *  auto parser = lookup(obj.value["type"].toString(),
     *                      "type1", fn1,
     *                      "type2", fn2,
     *                      fallbackFn);
     *  parser(obj);
     * }
     *
     * The implementation is based on tail recursion; every recursion step
     * removes 2 arguments (match and value). There's no selector value for the
     * fallback option (the last one); therefore, the total number of lookup()
     * arguments should be even: selector + n key-value pairs + fallback
     *
     * @note Beware of calling lookup() with a <code>const char*</code> selector
     * (the first parameter) - most likely it won't do what you expect because
     * of shallow comparison.
     */
    template <typename ValueT, typename SelectorT>
    ValueT lookup(SelectorT/*unused*/, ValueT&& fallback)
    {
        return std::forward<ValueT>(fallback);
    }

    template <typename ValueT, typename SelectorT, typename KeyT, typename... Ts>
    ValueT lookup(SelectorT&& selector, KeyT&& key, ValueT&& value, Ts&&... remainder)
    {
        if( selector == key )
            return std::forward<ValueT>(value);

        // Drop the failed key-value pair and recurse with 2 arguments less.
        return lookup<ValueT>(std::forward<SelectorT>(selector),
                              std::forward<Ts>(remainder)...);
    }

    /**
     * A wrapper around lookup() for functions of different types castable
     * to a common std::function<> form
     *
     * This class uses std::function<> magic to first capture arguments of
     * a yet-unknown function or function object, and then to coerce types of
     * all functions/function objects passed for lookup to the type
     * std::function<ResultT(ArgTs...). Without Dispatch<>, you would have
     * to pass the specific function type to lookup, since your functions have
     * different signatures. The type is not always obvious, and the resulting
     * construct in client code would almost always be rather cumbersome.
     * Dispatch<> deduces the necessary function type (well, almost - you still
     * have to specify the result type) and hides the clumsiness. For more
     * information on what std::function<> can wrap around, see
     * https://cpptruths.blogspot.jp/2015/11/covariance-and-contravariance-in-c.html
     *
     * The function arguments are captured by value (i.e. copied) to avoid
     * hard-to-find issues with dangling references in cases when a Dispatch<>
     * object is passed across different contexts (e.g. returned from another
     * function).
     *
     * \tparam ResultT - the desired type of a picked function invocation (mandatory)
     * \tparam ArgTs - function argument types (deduced)
     */
#if __GNUC__ < 5 && __GNUC_MINOR__ < 9
    // GCC 4.8 cannot cope with parameter packs inside lambdas; so provide a single
    // argument version of Dispatch<> that we only need so far.
    template <typename ResultT, typename ArgT>
#else
    template <typename ResultT, typename... ArgTs>
#endif
    class Dispatch
    {
            // The implementation takes a chapter from functional programming:
            // Dispatch<> uses a function that in turn accepts a function as its
            // argument. The sole purpose of the outer function (initialized by
            // a lambda-expression in the constructor) is to store the arguments
            // to any of the functions later looked up. The inner function (its
            // type is defined by fn_t alias) is the one returned by lookup()
            // invocation inside to().
            //
            // It's a bit counterintuitive to specify function parameters before
            // the list of functions but otherwise it would take several overloads
            // here to match all the ways a function-like behaviour can be done:
            // reference-to-function, pointer-to-function, function object. This
            // probably could be done as well but I preferred a more compact
            // solution: you show what you have and if it's possible to bring all
            // your functions to the same std::function<> based on what you have
            // as parameters, the code will compile. If it's not possible, modern
            // compilers are already good enough at pinpointing a specific place
            // where types don't match.
        public:
#if __GNUC__ < 5 && __GNUC_MINOR__ < 9
            using fn_t = std::function<ResultT(ArgT)>;
            explicit Dispatch(ArgT&& arg)
                : boundArgs([=](fn_t &&f) { return f(std::move(arg)); })
            { }
#else
            using fn_t = std::function<ResultT(ArgTs...)>;
            explicit Dispatch(ArgTs&&... args)
                : boundArgs([=](fn_t &&f) { return f(std::move(args)...); })
            { }
#endif

            template <typename... LookupParamTs>
            ResultT to(LookupParamTs&&... lookupParams)
            {
                // Here's the magic, two pieces of it:
                // 1. Specifying fn_t in lookup() wraps all functions in
                // \p lookupParams into the same std::function<> type. This
                // includes conversion of return types from more specific to more
                // generic (because std::function is covariant by return types and
                // contravariant by argument types (see the link in the Doxygen
                // part of the comments).
                auto fn = lookup<fn_t>(std::forward<LookupParamTs>(lookupParams)...);
                // 2. Passing the result of lookup() to boundArgs() invokes the
                // lambda-expression mentioned in the constructor, which simply
                // invokes this passed function with a set of arguments captured
                // by lambda.
                if (fn)
                    return boundArgs(std::move(fn));

                // A shortcut to allow passing nullptr for a function;
                // a default-constructed ResultT will be returned
                // (for pointers, it will be nullptr)
                return {};
            }

        private:
            std::function<ResultT(fn_t&&)> boundArgs;
    };

    /**
     * Dispatch a set of parameters to one of a set of functions, depending on
     * a selector value
     *
     * Use <code>dispatch<CommonType>(parameters).to(lookup parameters)</code>
     * instead of lookup() if you need to pick one of several functions returning
     * types castable to the same CommonType. See event.cpp for a typical use case.
     *
     * \see Dispatch
     */
    template <typename ResultT, typename... ArgTs>
    Dispatch<ResultT, ArgTs...> dispatch(ArgTs&& ... args)
    {
        return Dispatch<ResultT, ArgTs...>(std::forward<ArgTs>(args)...);
    }

    // The below enables pretty-printing of enums in logs
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
#define REGISTER_ENUM(EnumName) Q_ENUM(EnumName)
#else
    // Thanks to Olivier for spelling it and for making Q_ENUM to replace it:
    // https://woboq.com/blog/q_enum.html
#define REGISTER_ENUM(EnumName) \
    Q_ENUMS(EnumName) \
    friend QDebug operator<<(QDebug dbg, EnumName val) \
    { \
        static int enumIdx = staticMetaObject.indexOfEnumerator(#EnumName); \
        return dbg << Event::staticMetaObject.enumerator(enumIdx).valueToKey(int(val)); \
    }
#endif
}  // namespace QMatrixClient

