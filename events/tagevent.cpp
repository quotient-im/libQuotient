#include "tagevent.h"

using namespace QMatrixClient;

TagRecord::TagRecord(const QJsonObject& json)
    : order(json.value("order").toString())
{ }

TagEvent::TagEvent(const QJsonObject& obj)
    : Event(Type::Tag, obj)
{
    Q_ASSERT(obj["type"].toString() == TypeId);
}

QStringList TagEvent::tagNames() const
{
    return tagsObject().keys();
}

QHash<QString, TagRecord> TagEvent::tags() const
{
    QHash<QString, TagRecord> result;
    auto allTags { tagsObject() };
    for (auto it = allTags.begin(); it != allTags.end(); ++ it)
        result.insert(it.key(), TagRecord(it.value().toObject()));
    return result;
}

bool TagEvent::isFavourite() const
{
    return tagsObject().contains("m.favourite");
}

bool TagEvent::isLowPriority() const
{
    return tagsObject().contains("m.lowpriority");
}

QJsonObject TagEvent::tagsObject() const
{
    return contentJson().value("tags").toObject();
}
