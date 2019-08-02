/******************************************************************************
 * Copyright (C) 2019 Kitsune Ral <kitsune-ral@users.sf.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#pragma once

#include "util.h"

#include <QtCore/QPointer>

namespace QMatrixClient {
namespace _impl {
    template <typename SenderT, typename SignalT, typename ContextT, typename... ArgTs>
    inline QMetaObject::Connection
    connectUntil(SenderT* sender, SignalT signal, ContextT* context,
                 std::function<bool(ArgTs...)> slot, Qt::ConnectionType connType)
    {
        // See https://bugreports.qt.io/browse/QTBUG-60339
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
        auto pc = std::make_shared<QMetaObject::Connection>();
#else
        auto pc = std::make_unique<QMetaObject::Connection>();
#endif
        auto& c = *pc; // Resolve a reference before pc is moved to lambda
        c = QObject::connect(
            sender, signal, context,
            [pc = std::move(pc), slot](ArgTs... args) {
                Q_ASSERT(*pc); // If it's been triggered, it should exist
                if (slot(std::forward<ArgTs>(args)...))
                    QObject::disconnect(*pc);
            },
            connType);
        return c;
    }
} // namespace _impl

template <typename SenderT, typename SignalT, typename ContextT, typename FunctorT>
inline auto connectUntil(SenderT* sender, SignalT signal, ContextT* context,
                         const FunctorT& slot,
                         Qt::ConnectionType connType = Qt::AutoConnection)
{
    return _impl::connectUntil(
        sender, signal, context,
        typename function_traits<FunctorT>::function_type(slot), connType);
}

/** Create a single-shot connection that triggers on the signal and
 * then self-disconnects
 *
 * Only supports DirectConnection type.
 */
template <typename SenderT, typename SignalT, typename ReceiverT, typename SlotT>
inline auto connectSingleShot(SenderT* sender, SignalT signal,
                              ReceiverT* receiver, SlotT slot)
{
    QMetaObject::Connection connection;
    connection = QObject::connect(sender, signal, receiver, slot,
                                  Qt::DirectConnection);
    Q_ASSERT(connection);
    QObject::connect(
        sender, signal, receiver,
        [connection] { QObject::disconnect(connection); }, Qt::DirectConnection);
    return connection;
}

/** A guard pointer that disconnects an interested object upon destruction
 * It's almost QPointer<> except that you have to initialise it with one
 * more additional parameter - a pointer to a QObject that will be
 * disconnected from signals of the underlying pointer upon the guard's
 * destruction.
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
} // namespace QMatrixClient
