TEMPLATE = app

include(libquotient.pri)

QT += testlib

CONFIG *= c++1z warn_on object_parallel_to_source

windows { CONFIG *= console }

SOURCES += tests/quotest.cpp

DISTFILES += \
    .valgrind.supp
