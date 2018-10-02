#ifndef ENCRYPTIONEVENT_H
#define ENCRYPTIONEVENT_H

#pragma once

#include "event.h"
#include "roomevent.h"

namespace QMatrixClient {
class EncryptionEvent : public RoomEvent {
 public:
  DEFINE_EVENT_TYPEID("m.room.encryption", EncryptionEvent)

  explicit EncryptionEvent(const QJsonObject& obj);

  QString algorithm() const { return _algorithm; }
  int rotationPeriodMs() { return _rotation_period_ms; }
  int rotationPeriodMsgs() { return _rotation_period_msgs; }

 private:
  QString _algorithm;
  int _rotation_period_ms;
  int _rotation_period_msgs;
};
}  // namespace QMatrixClient

#endif  // ENCRYPTIONEVENT_H
