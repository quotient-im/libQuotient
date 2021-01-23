// SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QByteArray>
#include <variant>

//! An in-bound group session is responsible for decrypting incoming
//! communication in a Megolm session.
struct QOlmInboundGroupSession
{
public:
    //! Creates a new instance of `OlmInboundGroupSession`.
    static std::variant<OlmInboundGroupSession, OlmGroupSessionError> create(const QString &key);
private:
    OlmInboundGroupSession *m_groupSession
    QByteArray m_buffer;
};

