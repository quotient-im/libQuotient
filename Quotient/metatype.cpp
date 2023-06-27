#include "metatype.h"

#include <QtCore/QLoggingCategory>
#include <QtCore/QCoreApplication>

using namespace Quotient;

Q_LOGGING_CATEGORY(METATYPES, "quotient.metatypes", QtInfoMsg)

AbstractMetaType::AbstractMetaType(QLatin1String loadableBaseClassName)
    : className(loadableBaseClassName)
{
    Q_ASSERT(className == LoadableBaseClassName);
}

AbstractMetaType::AbstractMetaType(const char* loadableClassName,
                                   AbstractMetaType* nearestBase,
                                   QLatin1String matrixType)
    : className(loadableClassName), baseType(nearestBase)
{
    Q_ASSERT(className != LoadableBaseClassName);
    Q_ASSERT(nearestBase != nullptr);

    nearestBase->addDerived(matrixType, this);
}

void AbstractMetaType::addDerived(QLatin1String matrixType,
                                  const AbstractMetaType* newType)
{
    if (const auto existing =
        std::find_if(derivedTypes.cbegin(), derivedTypes.cend(),
                     [&newType](const auto& t) {
                         return t->className == newType->className;
                         });
        existing != derivedTypes.cend()) {
        if (*existing == newType)
            return;

        // One loadable class cannot have two different metatype objects
        if ((*existing)->className == newType->className)
            qFatal("%s is registered repeatedly; check this name for "
                   "copy-pastas and make sure the class is exported across "
                   "translation units or shared objects",
                   newType->className.data());
    }
    derivedTypes.emplace_back(newType);
    if (matrixType.isEmpty())
        qDebug(METATYPES).nospace().noquote()
            << "Generic loadable type " << className << " registered";
    else
        qDebug(METATYPES).nospace().noquote()
            << '"' << matrixType << "\" -> " << newType->className << "; "
            << derivedTypes.size() << " derived type(s) registered for "
            << className;
}
