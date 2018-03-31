QT += network
CONFIG += c++14 warn_on rtti_off create_prl

win32-msvc* {
    QMAKE_CXXFLAGS_WARN_ON += -wd4100
} else {
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
}

INCLUDEPATH += $$PWD/lib

HEADERS += \
    $$PWD/lib/connectiondata.h \
    $$PWD/lib/connection.h \
    $$PWD/lib/room.h \
    $$PWD/lib/user.h \
    $$PWD/lib/avatar.h \
    $$PWD/lib/util.h \
    $$PWD/lib/events/event.h \
    $$PWD/lib/events/eventcontent.h \
    $$PWD/lib/events/roommessageevent.h \
    $$PWD/lib/events/simplestateevents.h \
    $$PWD/lib/events/roommemberevent.h \
    $$PWD/lib/events/roomavatarevent.h \
    $$PWD/lib/events/typingevent.h \
    $$PWD/lib/events/receiptevent.h \
    $$PWD/lib/events/accountdataevents.h \
    $$PWD/lib/events/directchatevent.h \
    $$PWD/lib/events/redactionevent.h \
    $$PWD/lib/jobs/requestdata.h \
    $$PWD/lib/jobs/basejob.h \
    $$PWD/lib/jobs/checkauthmethods.h \
    $$PWD/lib/jobs/passwordlogin.h \
    $$PWD/lib/jobs/sendeventjob.h \
    $$PWD/lib/jobs/postreceiptjob.h \
    $$PWD/lib/jobs/joinroomjob.h \
    $$PWD/lib/jobs/roommessagesjob.h \
    $$PWD/lib/jobs/syncjob.h \
    $$PWD/lib/jobs/mediathumbnailjob.h \
    $$PWD/lib/jobs/setroomstatejob.h \
    $$PWD/lib/jobs/downloadfilejob.h \
    $$PWD/lib/jobs/postreadmarkersjob.h \
    $$files($$PWD/lib/jobs/generated/*.h, false) \
    $$PWD/lib/logging.h \
    $$PWD/lib/settings.h \
    $$PWD/lib/networksettings.h \
    $$PWD/lib/networkaccessmanager.h

SOURCES += \
    $$PWD/lib/connectiondata.cpp \
    $$PWD/lib/connection.cpp \
    $$PWD/lib/room.cpp \
    $$PWD/lib/user.cpp \
    $$PWD/lib/avatar.cpp \
    $$PWD/lib/events/event.cpp \
    $$PWD/lib/events/eventcontent.cpp \
    $$PWD/lib/events/roommessageevent.cpp \
    $$PWD/lib/events/roommemberevent.cpp \
    $$PWD/lib/events/typingevent.cpp \
    $$PWD/lib/events/receiptevent.cpp \
    $$PWD/lib/events/directchatevent.cpp \
    $$PWD/lib/jobs/requestdata.cpp \
    $$PWD/lib/jobs/basejob.cpp \
    $$PWD/lib/jobs/checkauthmethods.cpp \
    $$PWD/lib/jobs/passwordlogin.cpp \
    $$PWD/lib/jobs/sendeventjob.cpp \
    $$PWD/lib/jobs/postreceiptjob.cpp \
    $$PWD/lib/jobs/joinroomjob.cpp \
    $$PWD/lib/jobs/roommessagesjob.cpp \
    $$PWD/lib/jobs/syncjob.cpp \
    $$PWD/lib/jobs/mediathumbnailjob.cpp \
    $$PWD/lib/jobs/setroomstatejob.cpp \
    $$PWD/lib/jobs/downloadfilejob.cpp \
    $$files($$PWD/lib/jobs/generated/*.cpp, false) \
    $$PWD/lib/logging.cpp \
    $$PWD/lib/settings.cpp \
    $$PWD/lib/networksettings.cpp \
    $$PWD/lib/networkaccessmanager.cpp
