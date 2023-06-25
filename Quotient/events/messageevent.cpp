#include "messageevent.h"

using namespace Quotient;

EventContent::TextRepresentation::TextRepresentation(const QJsonObject& json)
{}

void EventContent::TextRepresentation::fillJson(QJsonObject &) const
{
}

MessageEvent::MessageEvent(const QJsonObject& json)
    : RoomEvent(json)
    , _textBlock(
          fromJson<std::vector<EventContent::TextRepresentation>>(contentJson()))
{

}
