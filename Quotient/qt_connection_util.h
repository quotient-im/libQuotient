// SPDX-FileCopyrightText: 2019 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "function_traits.h"

#include <QtCore/QObject>
#include <QtCore/QPointer>

namespace Quotient {
namespace _impl {
    enum ConnectionType { SingleShot, Until };

    template <ConnectionType CType>
    inline auto connect(auto* sender, auto signal, auto* context, auto slotLike,
                        Qt::ConnectionType connType)
    {
        auto pConn = std::make_unique<QMetaObject::Connection>();
        auto& c = *pConn; // Save the reference before pConn is moved from
        c = QObject::connect(
            sender, signal, context,
            [slotLike, pConn = std::move(pConn)](const auto&... args)
            // The requires-expression below is necessary to prevent Qt
            // from eagerly trying to fill the lambda with more arguments
            // than slotLike() (i.e., the original slot) can handle
            requires requires { slotLike(args...); } {
                static_assert(CType == Until || CType == SingleShot,
                              "Unsupported disconnection type");
                if constexpr (CType == SingleShot) {
                    // Disconnect early to avoid re-triggers during slotLike()
                    QObject::disconnect(*pConn);
                    // Qt kindly keeps slot objects until they do their job,
                    // even if they disconnect themselves in the process (see
                    // how doActivate() in qobject.cpp handles c->slotObj).
                    slotLike(args...);
                } else if constexpr (CType == Until) {
                    if (slotLike(args...))
                        QObject::disconnect(*pConn);
                }
            },
            connType);
        return c;
    }

    template <typename SlotT, typename ReceiverT>
    concept PmfSlot =
        (fn_arg_count_v<SlotT> > 0
         && std::is_base_of_v<std::decay_t<fn_arg_t<SlotT, 0>>, ReceiverT>);
} // namespace _impl

//! \brief Create a connection that self-disconnects when its slot returns true
//!
//! A slot accepted by connectUntil() is different from classic Qt slots
//! in that its return value must be bool, not void. Because of that different
//! signature connectUntil() doesn't accept member functions in the way
//! QObject::connect or Quotient::connectSingleShot do; you should pass a lambda
//! or a pre-bound member function to it.
//! \return whether the connection should be dropped; false means that the
//!         connection remains; upon returning true, the slot is disconnected
//!         from the signal.
inline auto connectUntil(auto* sender, auto signal, auto* context,
                         auto smartSlot,
                         Qt::ConnectionType connType = Qt::AutoConnection)
{
    return _impl::connect<_impl::Until>(sender, signal, context, smartSlot,
                                        connType);
}

//! Create a connection that self-disconnects after triggering on the signal
template <typename ContextT, typename SlotT>
inline auto connectSingleShot(auto* sender, auto signal, ContextT* context,
                              SlotT slot,
                              Qt::ConnectionType connType = Qt::AutoConnection)
{
    return QObject::connect(sender, signal, context, slot,
                            Qt::ConnectionType(connType
                                               | Qt::SingleShotConnection));
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
