TEMPLATE = app

QT += testlib
CONFIG *= c++1z warn_on object_parallel_to_source

windows { CONFIG *= console }

include(libquotient.pri)

SOURCES += tests/quotest.cpp

DISTFILES += \
    .valgrind.supp
