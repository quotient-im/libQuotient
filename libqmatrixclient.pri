QT += network
CONFIG += c++14 warn_on rtti_off create_prl

win32-msvc* {
    QMAKE_CXXFLAGS_WARN_ON += -wd4100
} else {
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
}

SRCPATH = $$PWD/lib
INCLUDEPATH += $$SRCPATH

HEADERS += \
    $$SRCPATH/connectiondata.h \
    $$SRCPATH/connection.h \
    $$SRCPATH/room.h \
    $$SRCPATH/user.h \
    $$SRCPATH/avatar.h \
    $$SRCPATH/util.h \
    $$SRCPATH/events/event.h \
    $$SRCPATH/events/eventcontent.h \
    $$SRCPATH/events/roommessageevent.h \
    $$SRCPATH/events/simplestateevents.h \
    $$SRCPATH/events/roommemberevent.h \
    $$SRCPATH/events/roomavatarevent.h \
    $$SRCPATH/events/typingevent.h \
    $$SRCPATH/events/receiptevent.h \
    $$SRCPATH/events/accountdataevents.h \
    $$SRCPATH/events/directchatevent.h \
    $$SRCPATH/events/redactionevent.h \
    $$SRCPATH/jobs/requestdata.h \
    $$SRCPATH/jobs/basejob.h \
    $$SRCPATH/jobs/sendeventjob.h \
    $$SRCPATH/jobs/postreceiptjob.h \
    $$SRCPATH/jobs/syncjob.h \
    $$SRCPATH/jobs/mediathumbnailjob.h \
    $$SRCPATH/jobs/downloadfilejob.h \
    $$SRCPATH/jobs/postreadmarkersjob.h \
    $$files($$SRCPATH/csapi/*.h, false) \
    $$files($$SRCPATH/csapi/definitions/*.h, false) \
    $$SRCPATH/logging.h \
    $$SRCPATH/converters.h \
    $$SRCPATH/settings.h \
    $$SRCPATH/networksettings.h \
    $$SRCPATH/networkaccessmanager.h

SOURCES += \
    $$SRCPATH/connectiondata.cpp \
    $$SRCPATH/connection.cpp \
    $$SRCPATH/room.cpp \
    $$SRCPATH/user.cpp \
    $$SRCPATH/avatar.cpp \
    $$SRCPATH/util.cpp \
    $$SRCPATH/events/event.cpp \
    $$SRCPATH/events/eventcontent.cpp \
    $$SRCPATH/events/roommessageevent.cpp \
    $$SRCPATH/events/roommemberevent.cpp \
    $$SRCPATH/events/typingevent.cpp \
    $$SRCPATH/events/receiptevent.cpp \
    $$SRCPATH/events/directchatevent.cpp \
    $$SRCPATH/jobs/requestdata.cpp \
    $$SRCPATH/jobs/basejob.cpp \
    $$SRCPATH/jobs/sendeventjob.cpp \
    $$SRCPATH/jobs/postreceiptjob.cpp \
    $$SRCPATH/jobs/syncjob.cpp \
    $$SRCPATH/jobs/mediathumbnailjob.cpp \
    $$SRCPATH/jobs/downloadfilejob.cpp \
    $$files($$SRCPATH/csapi/*.cpp, false) \
    $$files($$SRCPATH/csapi/definitions/*.cpp, false) \
    $$SRCPATH/logging.cpp \
    $$SRCPATH/converters.cpp \
    $$SRCPATH/settings.cpp \
    $$SRCPATH/networksettings.cpp \
    $$SRCPATH/networkaccessmanager.cpp
