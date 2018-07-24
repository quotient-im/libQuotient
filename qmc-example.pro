TEMPLATE = app

CONFIG += object_parallel_to_source

windows { CONFIG += console }

include(libqmatrixclient.pri)

SOURCES += examples/qmc-example.cpp

DISTFILES += \
    .valgrind.qmc-example.supp
