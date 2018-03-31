TEMPLATE = app

windows { CONFIG += console }

include(libqmatrixclient.pri)

SOURCES += examples/qmc-example.cpp

DISTFILES += \
    .valgrind.qmc-example.supp
