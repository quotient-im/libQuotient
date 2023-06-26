#pragma once

#include "function_traits.h"
#include "util.h"

#include <QtCore/QJsonObject>

namespace Quotient {

class LoadableBase;

using matrix_type_t = QLatin1String;

//! \brief The base class for metatypes
//!
//! You should not normally have to use this directly, unless you need to devise
//! a whole new kind of metatypes.
class QUOTIENT_API AbstractMetaType {
public:
    // The public fields here are const and are not to be changeable anyway.
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    const QLatin1String className; ///< C++ class name
    const AbstractMetaType* const baseType = nullptr;
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    explicit AbstractMetaType(QLatin1String loadableBaseClassName);
    explicit AbstractMetaType(const char* loadableClassName,
                              AbstractMetaType* nearestBase,
                              QLatin1String matrixType = {});

    void addDerived(QLatin1String matrixType, const AbstractMetaType* newType);

    virtual ~AbstractMetaType() = default;
    Q_DISABLE_COPY_MOVE(AbstractMetaType)

protected:
    template <class T>
    friend class MetaType;

    // The returned value indicates whether a generic object has to be created
    // on the top level when `result` is empty, instead of returning nullptr
    virtual bool doLoadFrom(const QJsonObject& fullJson, const QString& type,
                            const QString& roomVersion,
                            LoadableBase*& result) const = 0;

private:
    static constexpr auto LoadableBaseClassName = "LoadableBase"_ls;

    std::vector<const AbstractMetaType*> derivedTypes{};
};

// Any metatype is unique (note Q_DISABLE_COPY_MOVE above) so can be identified
// by its address
inline bool operator==(const AbstractMetaType& lhs, const AbstractMetaType& rhs)
{
    return &lhs == &rhs;
}

template <class T, class BaseT = LoadableBase>
concept Loadable_Class =
    std::is_base_of_v<BaseT, T> && std::is_base_of_v<LoadableBase, BaseT>;

//! \brief Non-member version of LoadableBase::is()
//!
//! The two are entirely equivalent; some might prefer LoadableBase::is() as
//! slightly more readable (`v.is<MessageEvent>()` is arguably more natural
//! order than `is<MessageEvent>(v)`) but the non-member version is much more
//! readable in templated calls (when `v` is a template you can still write
//! `is<MessageEvent>(v)` while the member call syntax becomes clumsy
//! `v.template is<MessageEvent>()`).
//! See LoadableBase::is for the explanation of what the function does.
template <class T, class BaseT>
inline bool is(const BaseT& v)
    requires Loadable_Class<T, BaseT>
{
    // Protect against accidental putting QUO_*EVENT to a private section
    static_assert(
        requires { &T::metaObject; },
        "Event class doesn't have a public metaObject() override - "
        "make sure QUO_*LOADABLE macro in in public section");
    if constexpr (requires { T::MetaObject; }) {
        return &v.metaObject() == &T::MetaObject;
    } else {
        const auto* p = &v.metaObject();
        do {
            if (p == &T::BaseMetaObject)
                return true;
        } while ((p = p->baseType) != nullptr);
        return false;
    }
}

//! \brief A family of types to load objects from JSON and match their types
//!
//! TL;DR for the loadFrom() story:
//! - for base types, use QUO_BASE_LOADABLE and, if you have additional
//!   validation (e.g., JSON has to contain a certain key - see StateEvent
//!   for a real example), define it in the static T::isValid() member function
//!   accepting QJsonObject and returning bool.
//! - for leaf (specific) types - simply use QUO_LOADABLE and it will do
//!   everything necessary.
//! \sa QUO_LOADABLE, QUO_BASE_LOADABLE
template <class T>
class QUOTIENT_API MetaType : public AbstractMetaType {
    // Above: can't constrain T to be EventClass because the definition of T
    // is incomplete at the point of MetaType<T> instantiation.
public:
    using AbstractMetaType::AbstractMetaType;

    //! \brief Try to load an object from JSON, with dynamic type resolution
    //!
    //! The generic logic defined in this class template and invoked applies to
    //! all types that have QUO_LOADABLE/QUO_BASE_LOADABLE in their definitions
    //! (event types and content block types, in particular) and boils down to
    //! the following:
    //! 1.
    //!    a. If T has TypeId defined (which normally is a case of all leaf -
    //!       specific - types, via QUO_LOADABLE macro) and \p type doesn't
    //!       exactly match it, nullptr is immediately returned.
    //!    b. In absence of TypeId, the most-specific base class for this object
    //!       is searched, recursively applying this very algorithm on each
    //!       metatype object stored in \p derivedTypes. E.g., if
    //!       `MetaType<Event>::loadFrom()` is called on JSON corresponding to
    //!       a member event then the algorithm will go from `MetaType<Event>`
    //!       to `MetaType<RoomEvent>`, then further to `MetaType<StateEvent>`
    //!       through pointers in respective\p derivedTypes, and then iterate
    //!       through specific \p derivedTypes in `MetaType<StateEvent>`
    //!       matching their `TypeId`s against the one stored in JSON, until
    //!       it successfully matches `RoomMemberEvent::TypeId`.
    //! 2. Optional validation: if T (or, due to the way inheritance works,
    //!    any of its base types) has a static isValid() predicate and the JSON
    //!    payload does not satisfy it, nullptr is immediately returned to
    //!    the upper level or to the loadFrom() caller. Taking the same example,
    //!    once `RoomMemberEvent::TypeId` is successfully matched, the algorithm
    //!    calls `RoomMemberEvent::isValid()` (which is actually inherited
    //!    from `StateEvent`) to validate that the payload contains the
    //!    mandatory `state_key` parameter.
    //! 3. If step 1b above returned non-nullptr, immediately return it.
    //! 4.
    //!    a. If T::isValid() or T::TypeId (either, or both) exist and
    //!       are satisfied (see steps 1a and 2 above) but step 1b returned
    //!       `nullptr`, an object of the current (deemed most-specific) type
    //!       is created from the passed JSON and returned. In case of a base
    //!       type with `isValid()`, this will be a generic (aka "unknown")
    //!       object.
    //!    b. If neither T::isValid() nor T::TypeId exists:
    //!       i. On top-level (the most generic type, on which `loadFrom()` was
    //!          called), a generic object is created and returned.
    //!       ii. On lower levels, `nullptr` is returned to the upper level and
    //!           the type lookup continues there. This is, e.g., a case of
    //!           a derived base event metatype (e.g. `MetaType<RoomEvent>`)
    //!           called from its base event metatype (`MetaType<Event>`). If
    //!           no matching type derived from RoomEvent is found, the nested
    //!           lookup returns nullptr rather than a generic RoomEvent,
    //!           so that other types derived from Event could be examined.
    std::unique_ptr<T> loadFrom(const QJsonObject& fullJson, const QString& type,
                                const QString& roomVersion = {}) const
    {
        LoadableBase* result = nullptr;
        const bool goodEnough = doLoadFrom(fullJson, type, roomVersion, result);
        if constexpr (!std::is_abstract_v<T>) {
            if (!result && goodEnough)
                return std::unique_ptr<T>{ new T(fullJson) }; // 4bi
        }
        return std::unique_ptr<T>{ static_cast<T*>(result) };
    }

private:
    bool doLoadFrom(const QJsonObject& fullJson, const QString& type,
                    const QString& roomVersion,
                    LoadableBase*& result) const override
    {
        if constexpr (requires { T::TypeId; }) { // 1a
            if (T::TypeId != type)
                return false;
        } else {
            for (const auto& p : this->derivedTypes) { // 1b
                p->doLoadFrom(fullJson, type, roomVersion, result);
                if (result) {
                    Q_ASSERT(is<T>(*result));
                    return false; // 3
                }
            }
        }
        if constexpr (std::is_abstract_v<T>) {
            return false;
        } else {
            if constexpr (requires { T::isValid; }) { // 2
                if (!T::isValid(fullJson))
                    return false;
            } else if constexpr (!requires { T::TypeId; }) {
                return true; // 4b; see loadFrom() for the return value treatment
            }
            // TypeId or isValid (or both) exist and either is satisfied
            result = new T(fullJson); // 4a
            return false;
        }
    }
};

template <>
class QUOTIENT_API MetaType<LoadableBase> : public AbstractMetaType {
public:
    MetaType() : AbstractMetaType(LoadableBaseClassName) {}

private:
    bool doLoadFrom(const QJsonObject&, const QString&, const QString&,
                    LoadableBase*&) const override
    {
        return false; // You can't create an object of type LoadableBase
    }
};

//! \brief Supply metatype information - base types
//!
//! Derive your base class from LoadableBase (directly or by deriving from
//! another base class that inherits LoadableBase) and use this macro in its
//! public section to provide type identity and enable creation of generic
//! objects of that type from JSON payloads. This macro provides BaseMetaObject
//! static field initialised by parameters passed to the macro, and
//! the metaObject() override pointing to that BaseMetaObject.
//! \note Do _not_ add this macro if your class is an intermediate wrapper that
//!       is not supposed to be instantiated on its own.
//! \sa MetaType
#define QUO_BASE_LOADABLE(CppType_, BaseCppType_)                     \
    friend class MetaType<CppType_>;                                  \
    static inline MetaType<CppType_> BaseMetaObject{                  \
        #CppType_, &BaseCppType_::BaseMetaObject                      \
    };                                                                \
    static_assert(&CppType_::BaseMetaObject == &BaseMetaObject,       \
                  #CppType_ " is wrong here - check for copy-pasta"); \
    const AbstractMetaType& metaObject() const override               \
    {                                                                 \
        return BaseMetaObject;                                        \
    }                                                                 \
    // End of macro

//! \brief Supply metatype information in (specific, leaf) types
//!
//! Derive your class from a base class that has QUO_BASE_LOADABLE macro, and
//! use this (QUO_LOADABLE) macro in its public sectio to provide type
//! identity and enable dynamic loading from JSON for objects of that type.
//! This macro provides MetaObject static field initialised as described below;
//! the metaObject() override pointing to it; and the TypeId static field that
//! keeps the Matrix id from which it can be loaded.
//!
//! The first two macro parameters are used as the first two MetaObject
//! constructor parameters; the third MetaObject parameter is set to
//! BaseMetaObject (which is why this macro requires deriving from a class that
//! defines BaseMetaObject, normally via QUO_BASE_LOADABLE macro).
//! \note Do _not_ use this macro if your class is an intermediate wrapper that
//!       is not supposed to be instantiated on its own.
//! \sa MetaType
#define QUO_LOADABLE(CppType_, MatrixType_)                                    \
    static inline const auto TypeId = MatrixType_##_ls;                        \
    friend class MetaType<CppType_>;                                           \
    static inline const MetaType<CppType_> MetaObject{ #CppType_,              \
                                                       &BaseMetaObject,        \
                                                       TypeId };               \
    static_assert(&CppType_::MetaObject == &MetaObject,                        \
                  #CppType_ " is wrong here - check for copy-pasta");          \
    const AbstractMetaType& metaObject() const override { return MetaObject; } \
    // End of macro

//! Facility function to get the most specific metatype object for a given type
template <class T>
constexpr const auto& mostSpecificMetaObject()
{
    if constexpr (requires { T::MetaObject; })
        return T::MetaObject;
    else
        return T::BaseMetaObject;
}

//! \brief Cast the pointer to a loadable down in a type-safe way
//!
//! Checks that \p p points to an object of the requested type and returns
//! a (plain) pointer downcast to that type. \p p can be either "dumb"
//! (`BaseT*`) or "smart" (`std::unique_ptr<BaseT>`). This overload doesn't
//! affect the object ownership - if the original pointer owns the object it
//! must outlive the downcast pointer to keep it from dangling.
template <class T, typename BasePtrT>
inline auto downcast(const BasePtrT& p)
    -> decltype(static_cast<T*>(std::to_address(p)))
{
    return p && is<std::decay_t<T>>(*p) ? static_cast<T*>(std::to_address(p))
                                        : nullptr;
}

//! \brief Cast the pointer to a loadable down in a type-safe way, with moving
//!
//! Checks that \p p points to an object of the requested type; if (and only
//! if) it is, releases the passed pointer, downcasts it to the requested object
//! type and returns a new unique pointer wrapping the downcast one. Unlike
//! the non-moving downcast(), this one only accepts a smart pointer, and that
//! smart pointer should be an rvalue (either a temporary, or as a result of
//! std::move()). The ownership, respectively, is transferred to the new
//! pointer; the original smart pointer is reset to nullptr, as is normal for
//! `unique_ptr<>::release()`.
//! \note If \p p points to an object of a type different from \p T it retains
//!       ownership after calling this overload; if it is a temporary, this
//!       normally leads to the object getting deleted along with the end of
//!       the temporary's lifetime.
template <class T, typename BaseT>
inline auto downcast(std::unique_ptr<BaseT>&& p)
{
    return p && is<std::decay_t<T>>(*p)
               ? std::unique_ptr<T>(static_cast<T*>(p.release()))
               : nullptr;
}

namespace _impl {
    template <typename FnT, typename BaseT>
    concept Invocable_With_Downcast =
        Loadable_Class<std::remove_cvref_t<fn_arg_t<FnT>>, BaseT>;
}

template <Loadable_Class BaseT, typename TailT>
inline auto switchOnType(const BaseT& loadable, TailT&& tail)
{
    if constexpr (std::is_invocable_v<TailT, BaseT>) {
        return tail(loadable);
    } else if constexpr (_impl::Invocable_With_Downcast<TailT, BaseT>) {
        using loadable_type = fn_arg_t<TailT>;
        if (is<std::decay_t<loadable_type>>(loadable))
            return tail(static_cast<loadable_type>(loadable));
        return std::invoke_result_t<TailT, loadable_type>(); // Default-constructed
    } else { // Treat it as a value to return
        return std::forward<TailT>(tail);
    }
}

//! \brief Non-member version of LoadableBase::switchOnType()
//!
//! See Quotient::is() for the discussion of member vs. non-member versions;
//! LoadableBase::switchOnType() for the semantics.
template <typename FnT1, typename... FnTs>
inline auto switchOnType(const Loadable_Class auto& loadable, FnT1&& fn1,
                         FnTs&&... fns)
{
    using loadable_type1 = fn_arg_t<FnT1>;
    if (is<std::decay_t<loadable_type1>>(loadable))
        return fn1(static_cast<loadable_type1>(loadable));
    return switchOnType(loadable, std::forward<FnTs>(fns)...);
}

// And finally...

class QUOTIENT_API LoadableBase {
public:
    // Provide the foundation for QUO_BASE_LOADABLE to work and common parts
    // for all "loadable" classes

    static inline MetaType<LoadableBase> BaseMetaObject{};
    virtual const AbstractMetaType& metaObject() const = 0;
    virtual ~LoadableBase() = default;

    //! \brief TODO
    template <class T>
    bool is() const
    {
        return Quotient::is<T>(*this);
    }

    //! \brief Apply one of the visitors based on the actual object type
    //!
    //! This function uses function_traits template and is() to find the first
    //! of the passed visitor invocables that can be called with this object
    //! object, downcasting `this` in a type-safe way to the most specific type
    //! accepted by the visitor. Without this function, you can still write
    //! a stack of, for example,
    //! `(else) if (const auto* evtPtr = downcast<...>(baseEvtPtr))`
    //! blocks but switchType() provides a more concise and isolating syntax:
    //! there's no `else` or trailing `return/break` to forget, for one.
    //! The visitors have to all return the same type (possibly void).
    //! Here's how you might use this function:
    //! \code
    //! RoomEventPtr eptr = /* get the event pointer from somewhere */;
    //! const auto result = eptr->switchOnType(
    //!     [](const RoomMemberEvent& memberEvent) {
    //!         // Do what's needed if eptr points to a RoomMemberEvent
    //!         return 1;
    //!     },
    //!     [](const CallEvent& callEvent) {
    //!         // Do what's needed if eptr points to a CallEvent or any
    //!         // class derived from it
    //!         return 2;
    //!     },
    //!     3); /* the default value to return if nothing above matched */
    //! \endcode
    //! As the example shows, the last parameter can optionally be
    //! a plain returned value instead of a visitor.
    template <typename... VisitorTs>
    auto switchOnType(VisitorTs&&... visitors) const
    {
        return Quotient::switchOnType(*this,
                                      std::forward<VisitorTs>(visitors)...);
    }
};

} // namespace Quotient
