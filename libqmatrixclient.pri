QT += network
CONFIG += c++11 warn_on rtti_off

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/connectiondata.h \
    $$PWD/connection.h \
    $$PWD/room.h \
    $$PWD/user.h \
    $$PWD/avatar.h \
    $$PWD/util.h \
    $$PWD/events/event.h \
    $$PWD/events/eventcontent.h \
    $$PWD/events/roommessageevent.h \
    $$PWD/events/roomnameevent.h \
    $$PWD/events/roomaliasesevent.h \
    $$PWD/events/roomcanonicalaliasevent.h \
    $$PWD/events/roommemberevent.h \
    $$PWD/events/roomtopicevent.h \
    $$PWD/events/roomavatarevent.h \
    $$PWD/events/typingevent.h \
    $$PWD/events/receiptevent.h \
    $$PWD/jobs/basejob.h \
    $$PWD/jobs/checkauthmethods.h \
    $$PWD/jobs/passwordlogin.h \
    $$PWD/jobs/sendeventjob.h \
    $$PWD/jobs/postreceiptjob.h \
    $$PWD/jobs/joinroomjob.h \
    $$PWD/jobs/roommessagesjob.h \
    $$PWD/jobs/syncjob.h \
    $$PWD/jobs/mediathumbnailjob.h \
    $$PWD/jobs/setroomstatejob.h \
    $$files($$PWD/jobs/generated/*.h, false) \
    $$PWD/logging.h \
    $$PWD/settings.h

SOURCES += \
    $$PWD/connectiondata.cpp \
    $$PWD/connection.cpp \
    $$PWD/room.cpp \
    $$PWD/user.cpp \
    $$PWD/avatar.cpp \
    $$PWD/events/event.cpp \
    $$PWD/events/eventcontent.cpp \
    $$PWD/events/roommessageevent.cpp \
    $$PWD/events/roomnameevent.cpp \
    $$PWD/events/roomaliasesevent.cpp \
    $$PWD/events/roomcanonicalaliasevent.cpp \
    $$PWD/events/roommemberevent.cpp \
    $$PWD/events/roomtopicevent.cpp \
    $$PWD/events/roomavatarevent.cpp \
    $$PWD/events/typingevent.cpp \
    $$PWD/events/receiptevent.cpp \
    $$PWD/jobs/basejob.cpp \
    $$PWD/jobs/checkauthmethods.cpp \
    $$PWD/jobs/passwordlogin.cpp \
    $$PWD/jobs/sendeventjob.cpp \
    $$PWD/jobs/postreceiptjob.cpp \
    $$PWD/jobs/joinroomjob.cpp \
    $$PWD/jobs/roommessagesjob.cpp \
    $$PWD/jobs/syncjob.cpp \
    $$PWD/jobs/mediathumbnailjob.cpp \
    $$PWD/jobs/setroomstatejob.cpp \
    $$files($$PWD/jobs/generated/*.cpp, false) \
    $$PWD/logging.cpp \
    $$PWD/settings.cpp
