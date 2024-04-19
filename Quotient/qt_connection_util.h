// SPDX-FileCopyrightText: 2019 Kitsune Ral <kitsune-ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

namespace Quotient {

//! \brief Create a connection that self-disconnects when its slot returns true
//!
//! The "slot" signature accepted by connectUntil() is different from normal Qt slots in that its
//! return value must be bool, not void. Also, as of this writing, connectUntil() doesn't accept
//! member function pointers for slots as QObject::connect or Quotient::connectSingleShot do; if
//! you want to use a member function of the context object you should pass a lambda or bind
//! the context to the member function with std::bind_front before passing it to connectUntil().
//! \return whether the connection should be dropped; false means that the
//!         connection remains; upon returning true, the slot is disconnected
//!         from the signal.
template <typename SmartSlotT>
inline auto connectUntil(auto* sender, auto signal, QObject* context, SmartSlotT&& smartSlot,
                         Qt::ConnectionType connType = Qt::AutoConnection)
{
    auto* cHolder = new QObject(context);
    // Apart from checking the "smart slot" return type, the 'requires' clause below prevents Qt
    // from eagerly trying to fill the lambda with more arguments than the "smart slot" can take
    return QObject::connect(
        sender, signal, cHolder,
        [sl = std::forward<SmartSlotT>(smartSlot), cHolder]<typename... Ts>
            requires std::invocable<SmartSlotT, Ts...>
        (const Ts&... args) {
            static_assert(std::is_same_v<decltype(sl(args...)), bool>);
            if (sl(args...))
                delete cHolder; // break the connection
        },
        connType);
}

//! Create a connection that self-disconnects after triggering on the signal
template <typename ContextT, typename SlotT>
inline auto connectSingleShot(auto* sender, auto signal, ContextT* context, SlotT slot,
                              Qt::ConnectionType connType = Qt::AutoConnection)
{
    return QObject::connect(sender, signal, context, slot,
                            Qt::ConnectionType(connType | Qt::SingleShotConnection));
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
