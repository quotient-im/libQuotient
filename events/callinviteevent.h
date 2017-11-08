/******************************************************************************
 * Copyright (C) 2017 Marius Gripsgard <marius@ubports.com>
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

#include "event.h"

namespace QMatrixClient
{
  class CallInviteEvent: public RoomEvent
  {
  public:
      explicit CallInviteEvent(const QJsonObject& obj);

      explicit CallInviteEvent(const QString& callId, const int lifetime,
                               const QString& sdp);

      const int lifetime() const { return _lifetime; }
      const QString& sdp() const { return _sdp; }
      const QString& callId() const { return _callId; }
      const int version() const { return _version; }

      QJsonObject toJson() const
      {
          QJsonObject offer;
          offer.insert("sdp", _sdp);
          offer.insert("type", QStringLiteral("offer"));

          QJsonObject obj;
          obj.insert("call_id", _callId);
          obj.insert("version", _version);
          obj.insert("lifetime", _lifetime);
          obj.insert("offer", offer);
          return obj;
      }

      static constexpr const char* TypeId = "m.call.invite";

  private:
      int _lifetime;
      QString _sdp;
      QString _callId;
      int _version;
  };
}
