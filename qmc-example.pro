TEMPLATE = app

windows { CONFIG += console }

include(libqmatrixclient.pri)

SOURCES += examples/qmc-example.cpp
