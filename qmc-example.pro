TEMPLATE = app

CONFIG *= c++1z warn_on object_parallel_to_source

windows { CONFIG *= console }

include(libquotient.pri)

SOURCES += examples/qmc-example.cpp

DISTFILES += \
    .valgrind.qmc-example.supp
