#pragma once

#include "roomevent.h"
#include "eventcontent.h"
#include "../metatype.h"

namespace Quotient {

namespace EventContent {
    class TextRepresentation : public Base {
    public:
        QUO_LOADABLE(TextRepresentation, "m.text")

        explicit TextRepresentation(const QJsonObject& json);

        QMimeType mimeType;
        QString body;

    private:
        void fillJson(QJsonObject&) const override;
    };
    using TextBlock = std::vector<TextRepresentation>;
}

inline std::unique_ptr<EventContent::Base> loadBlock(const QString& type,
                                                     const QJsonObject& json)
{
    return EventContent::Base::BaseMetaObject.loadFrom(json, type);
}


class QUOTIENT_API MessageEvent : public RoomEvent {
public:
    QUO_EVENT(MessageEvent, "org.matrix.msc1767.message") // "m.message"?

    const EventContent::TextBlock& textBlock() const { return _textBlock; }

protected:
    explicit MessageEvent(const QJsonObject& json);

private:
    EventContent::TextBlock _textBlock;
};
} // namespace Quotient
