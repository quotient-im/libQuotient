#include "encryptionevent.h"

using namespace QMatrixClient;

EncryptionEvent::EncryptionEvent(const QJsonObject& obj)
    : RoomEvent(typeId(), matrixTypeId(), contentJson()),
      _algorithm(contentJson()["algorithm"].toString()),
      _rotation_period_ms(contentJson()["rotation_period_ms"].toInt(604800000)),
      _rotation_period_msgs(contentJson()["rotation_period_msgs"].toInt(100)) {}
