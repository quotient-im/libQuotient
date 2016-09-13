QT += network
CONFIG += c++11

INCLUDEPATH += $$PWD $$PWD/kcoreaddons/src/lib/jobs

HEADERS += \
    $$PWD/connectiondata.h \
    $$PWD/connection.h \
    $$PWD/room.h \
    $$PWD/user.h \
    $$PWD/logmessage.h \
    $$PWD/state.h \
    $$PWD/events/event.h \
    $$PWD/events/roommessageevent.h \
    $$PWD/events/roomnameevent.h \
    $$PWD/events/roomaliasesevent.h \
    $$PWD/events/roomcanonicalaliasevent.h \
    $$PWD/events/roommemberevent.h \
    $$PWD/events/roomtopicevent.h \
    $$PWD/events/typingevent.h \
    $$PWD/events/receiptevent.h \
    $$PWD/events/unknownevent.h \
    $$PWD/jobs/basejob.h \
    $$PWD/jobs/checkauthmethods.h \
    $$PWD/jobs/passwordlogin.h \
    $$PWD/jobs/postmessagejob.h \
    $$PWD/jobs/postreceiptjob.h \
    $$PWD/jobs/joinroomjob.h \
    $$PWD/jobs/leaveroomjob.h \
    $$PWD/jobs/roommembersjob.h \
    $$PWD/jobs/roommessagesjob.h \
    $$PWD/jobs/syncjob.h \
    $$PWD/jobs/mediathumbnailjob.h \
    $$PWD/jobs/logoutjob.h

SOURCES += \
    $$PWD/connectiondata.cpp \
    $$PWD/connection.cpp \
    $$PWD/room.cpp \
    $$PWD/user.cpp \
    $$PWD/logmessage.cpp \
    $$PWD/state.cpp \
    $$PWD/events/event.cpp \
    $$PWD/events/roommessageevent.cpp \
    $$PWD/events/roomnameevent.cpp \
    $$PWD/events/roomaliasesevent.cpp \
    $$PWD/events/roomcanonicalaliasevent.cpp \
    $$PWD/events/roommemberevent.cpp \
    $$PWD/events/roomtopicevent.cpp \
    $$PWD/events/typingevent.cpp \
    $$PWD/events/receiptevent.cpp \
    $$PWD/events/unknownevent.cpp \
    $$PWD/jobs/basejob.cpp \
    $$PWD/jobs/checkauthmethods.cpp \
    $$PWD/jobs/passwordlogin.cpp \
    $$PWD/jobs/postmessagejob.cpp \
    $$PWD/jobs/postreceiptjob.cpp \
    $$PWD/jobs/joinroomjob.cpp \
    $$PWD/jobs/leaveroomjob.cpp \
    $$PWD/jobs/roommembersjob.cpp \
    $$PWD/jobs/roommessagesjob.cpp \
    $$PWD/jobs/syncjob.cpp \
    $$PWD/jobs/mediathumbnailjob.cpp \
    $$PWD/jobs/logoutjob.cpp
