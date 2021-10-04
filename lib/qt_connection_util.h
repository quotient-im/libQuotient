// SPDX-FileCopyrightText: 2019 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "util.h"

#include <QtCore/QPointer>

namespace Quotient {
namespace _impl {
    template <typename... ArgTs>
    using decorated_slot_tt =
        std::function<void(QMetaObject::Connection&, const ArgTs&...)>;

    template <typename SenderT, typename SignalT, typename ContextT, typename... ArgTs>
    inline QMetaObject::Connection
    connectDecorated(SenderT* sender, SignalT signal, ContextT* context,
                     decorated_slot_tt<ArgTs...> decoratedSlot,
                     Qt::ConnectionType connType)
    {
        // See https://bugreports.qt.io/browse/QTBUG-60339
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
        auto pc = std::make_shared<QMetaObject::Connection>();
#else
        auto pc = std::make_unique<QMetaObject::Connection>();
#endif
        auto& c = *pc; // Resolve a reference before pc is moved to lambda

        // Perfect forwarding doesn't work through signal-slot connections -
        // arguments are always copied (at best - COWed) to the context of
        // the slot. Therefore the slot decorator receives const ArgTs&...
        // rather than ArgTs&&...
        // TODO (C++20): std::bind_front() instead of lambda.
        c = QObject::connect(sender, signal, context,
            [pc = std::move(pc),
             decoratedSlot = std::move(decoratedSlot)](const ArgTs&... args) {
                Q_ASSERT(*pc); // If it's been triggered, it should exist
                decoratedSlot(*pc, args...);
            },
            connType);
        return c;
    }
    template <typename SenderT, typename SignalT, typename ContextT,
              typename... ArgTs>
    inline QMetaObject::Connection
    connectUntil(SenderT* sender, SignalT signal, ContextT* context,
                 std::function<bool(ArgTs...)> functor,
                 Qt::ConnectionType connType)
    {
        return connectDecorated(sender, signal, context,
            decorated_slot_tt<ArgTs...>(
                [functor = std::move(functor)](QMetaObject::Connection& c,
                                               const ArgTs&... args) {
                    if (functor(args...))
                        QObject::disconnect(c);
                }),
            connType);
    }
    template <typename SenderT, typename SignalT, typename ContextT,
              typename... ArgTs>
    inline QMetaObject::Connection
    connectSingleShot(SenderT* sender, SignalT signal, ContextT* context,
                      std::function<void(ArgTs...)> slot,
                      Qt::ConnectionType connType)
    {
        return connectDecorated(sender, signal, context,
            decorated_slot_tt<ArgTs...>(
                [slot = std::move(slot)](QMetaObject::Connection& c,
                                         const ArgTs&... args) {
                    QObject::disconnect(c);
                    slot(args...);
                }),
            connType);
    }

    // TODO: get rid of it as soon as Apple Clang gets proper deduction guides
    //       for std::function<>
    //       ...or consider using QtPrivate magic used by QObject::connect()
    //       ...for inspiration, also check a possible std::not_fn implementation
    //       at https://en.cppreference.com/w/cpp/utility/functional/not_fn
    template <typename FnT>
    inline auto wrap_in_function(FnT&& f)
    {
        return typename function_traits<FnT>::function_type(std::forward<FnT>(f));
    }
} // namespace _impl

/*! \brief Create a connection that self-disconnects when its "slot" returns true
 *
 *  A slot accepted by connectUntil() is different from classic Qt slots
 * in that its return value must be bool, not void. The slot's return value
 * controls whether the connection should be kept; if the slot returns false,
 * the connection remains; upon returning true, the slot is disconnected from
 * the signal. Because of a different slot signature connectUntil() doesn't
 * accept member functions as QObject::connect or Quotient::connectSingleShot
 * do; you should pass a lambda or a pre-bound member function to it.
 */
template <typename SenderT, typename SignalT, typename ContextT, typename FunctorT>
inline auto connectUntil(SenderT* sender, SignalT signal, ContextT* context,
                         const FunctorT& slot,
                         Qt::ConnectionType connType = Qt::AutoConnection)
{
    return _impl::connectUntil(sender, signal, context, _impl::wrap_in_function(slot),
                               connType);
}

/// Create a connection that self-disconnects after triggering on the signal
template <typename SenderT, typename SignalT, typename ContextT, typename FunctorT>
inline auto connectSingleShot(SenderT* sender, SignalT signal,
                              ContextT* context, const FunctorT& slot,
                              Qt::ConnectionType connType = Qt::AutoConnection)
{
    return _impl::connectSingleShot(
        sender, signal, context, _impl::wrap_in_function(slot), connType);
}

// Specialisation for usual Qt slots passed as pointers-to-members.
template <typename SenderT, typename SignalT, typename ReceiverT,
          typename SlotObjectT, typename... ArgTs>
inline auto connectSingleShot(SenderT* sender, SignalT signal,
                              ReceiverT* receiver,
                              void (SlotObjectT::*slot)(ArgTs...),
                              Qt::ConnectionType connType = Qt::AutoConnection)
{
    // TODO: when switching to C++20, use std::bind_front() instead
    return _impl::connectSingleShot(sender, signal, receiver,
                                    _impl::wrap_in_function(
                                        [receiver, slot](const ArgTs&... args) {
                                            (receiver->*slot)(args...);
                                        }),
                                    connType);
}

/*! \brief A guard pointer that disconnects an interested object upon destruction
 *
 * It's almost QPointer<> except that you have to initialise it with one
 * more additional parameter - a pointer to a QObject that will be
 * disconnected from signals of the underlying pointer upon the guard's
 * destruction. Note that destructing the guide doesn't destruct either QObject.
 */
template <typename T>
class ConnectionsGuard : public QPointer<T> {
public:
    ConnectionsGuard(T* publisher, QObject* subscriber)
        : QPointer<T>(publisher), subscriber(subscriber)
    {}
    ~ConnectionsGuard()
    {
        if (*this)
            (*this)->disconnect(subscriber);
    }
    ConnectionsGuard(ConnectionsGuard&&) = default;
    ConnectionsGuard& operator=(ConnectionsGuard&&) = default;
    Q_DISABLE_COPY(ConnectionsGuard)
    using QPointer<T>::operator=;

private:
    QObject* subscriber;
};
} // namespace Quotient
