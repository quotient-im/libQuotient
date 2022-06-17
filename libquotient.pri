QT += network multimedia
QT -= gui

CONFIG *= c++20 warn_on rtti_off create_prl object_parallel_to_source

win32-msvc* {
    # Quotient code base does not play well with NMake inference rules
    CONFIG *= no_batch
    QMAKE_CXXFLAGS_WARN_ON *= -wd4100 -wd4267
    QMAKE_CXXFLAGS *= /std:c++17 # Older Qt doesn't understand c++1z above
} else {
    QMAKE_CXXFLAGS_WARN_ON *= -Wno-unused-parameter
}

DEFINES += QT_NO_JAVA_STYLE_ITERATORS
contains(DEFINES, Quotient_E2EE_ENABLED=.) {
    contains(DEFINES, USE_INTREE_LIBQOLM=.) {
        include(3rdparty/libQtOlm/libQtOlm.pri)
    } else {
        CONFIG += link_pkgconfig
        PKGCONFIG += QtOlm
    }
}

SRCPATH = $$PWD/lib
INCLUDEPATH += $$SRCPATH

HEADERS += \
    $$SRCPATH/connectiondata.h \
    $$SRCPATH/connection.h \
    $$SRCPATH/ssosession.h \
    $$SRCPATH/encryptionmanager.h \
    $$SRCPATH/eventitem.h \
    $$SRCPATH/room.h \
    $$SRCPATH/user.h \
    $$SRCPATH/avatar.h \
    $$SRCPATH/uri.h \
    $$SRCPATH/uriresolver.h \
    $$SRCPATH/syncdata.h \
    $$SRCPATH/quotient_common.h \
    $$SRCPATH/util.h \
    $$SRCPATH/qt_connection_util.h \
    $$SRCPATH/events/event.h \
    $$SRCPATH/events/roomevent.h \
    $$SRCPATH/events/stateevent.h \
    $$SRCPATH/events/eventcontent.h \
    $$SRCPATH/events/roommessageevent.h \
    $$SRCPATH/events/simplestateevents.h \
    $$SRCPATH/events/roomcanonicalaliasevent.h \
    $$SRCPATH/events/roomcreateevent.h \
    $$SRCPATH/events/roomtombstoneevent.h \
    $$SRCPATH/events/roommemberevent.h \
    $$SRCPATH/events/roomavatarevent.h \
    $$SRCPATH/events/typingevent.h \
    $$SRCPATH/events/receiptevent.h \
    $$SRCPATH/events/reactionevent.h \
    $$SRCPATH/events/callanswerevent.h \
    $$SRCPATH/events/callcandidatesevent.h \
    $$SRCPATH/events/callhangupevent.h \
    $$SRCPATH/events/callinviteevent.h \
    $$SRCPATH/events/accountdataevents.h \
    $$SRCPATH/events/directchatevent.h \
    $$SRCPATH/events/encryptionevent.h \
    $$SRCPATH/events/encryptedevent.h \
    $$SRCPATH/events/roomkeyevent.h \
    $$SRCPATH/events/redactionevent.h \
    $$SRCPATH/events/eventloader.h \
    $$SRCPATH/events/roompowerlevelsevent.h \
    $$SRCPATH/jobs/requestdata.h \
    $$SRCPATH/jobs/basejob.h \
    $$SRCPATH/jobs/syncjob.h \
    $$SRCPATH/jobs/mediathumbnailjob.h \
    $$SRCPATH/jobs/downloadfilejob.h \
    $$files($$SRCPATH/csapi/*.h, false) \
    $$files($$SRCPATH/csapi/definitions/*.h, false) \
    $$files($$SRCPATH/csapi/definitions/wellknown/*.h, false) \
    $$files($$SRCPATH/application-service/definitions/*.h, false) \
    $$files($$SRCPATH/identity/definitions/*.h, false) \
    $$SRCPATH/logging.h \
    $$SRCPATH/converters.h \
    $$SRCPATH/settings.h \
    $$SRCPATH/networksettings.h \
    $$SRCPATH/networkaccessmanager.h

SOURCES += \
    $$SRCPATH/connectiondata.cpp \
    $$SRCPATH/connection.cpp \
    $$SRCPATH/ssosession.cpp \
    $$SRCPATH/encryptionmanager.cpp \
    $$SRCPATH/eventitem.cpp \
    $$SRCPATH/room.cpp \
    $$SRCPATH/user.cpp \
    $$SRCPATH/avatar.cpp \
    $$SRCPATH/uri.cpp \
    $$SRCPATH/uriresolver.cpp \
    $$SRCPATH/syncdata.cpp \
    $$SRCPATH/util.cpp \
    $$SRCPATH/events/event.cpp \
    $$SRCPATH/events/roomevent.cpp \
    $$SRCPATH/events/stateevent.cpp \
    $$SRCPATH/events/eventcontent.cpp \
    $$SRCPATH/events/roomcreateevent.cpp \
    $$SRCPATH/events/roomtombstoneevent.cpp \
    $$SRCPATH/events/roommessageevent.cpp \
    $$SRCPATH/events/roommemberevent.cpp \
    $$SRCPATH/events/typingevent.cpp \
    $$SRCPATH/events/reactionevent.cpp \
    $$SRCPATH/events/callanswerevent.cpp \
    $$SRCPATH/events/callcandidatesevent.cpp \
    $$SRCPATH/events/callhangupevent.cpp \
    $$SRCPATH/events/callinviteevent.cpp \
    $$SRCPATH/events/receiptevent.cpp \
    $$SRCPATH/events/directchatevent.cpp \
    $$SRCPATH/events/encryptionevent.cpp \
    $$SRCPATH/events/encryptedevent.cpp \
    $$SRCPATH/events/roomkeyevent.cpp \
    $$SRCPATH/events/roompowerlevelsevent.cpp \
    $$SRCPATH/jobs/requestdata.cpp \
    $$SRCPATH/jobs/basejob.cpp \
    $$SRCPATH/jobs/syncjob.cpp \
    $$SRCPATH/jobs/mediathumbnailjob.cpp \
    $$SRCPATH/jobs/downloadfilejob.cpp \
    $$files($$SRCPATH/csapi/*.cpp, false) \
    $$SRCPATH/logging.cpp \
    $$SRCPATH/converters.cpp \
    $$SRCPATH/settings.cpp \
    $$SRCPATH/networksettings.cpp \
    $$SRCPATH/networkaccessmanager.cpp
