set(Quotient_HEADERS
    avatar.h
    connection.h
    connectiondata.h
    converters.h
    eventitem.h
    e2ee.h
    logging.h
    encryptionmanager.h
    networkaccessmanager.h
    networksettings.h
    qt_connection_util.h
    quotient_common.h
    settings.h
    joinstate.h
    ssosession.h
    syncdata.h
    user.h
    uri.h
    uriresolver.h
    util.h

    # jobs
    jobs/basejob.h
    jobs/syncjob.h
    jobs/requestdata.h
    jobs/downloadfilejob.h
    jobs/mediathumbnailjob.h

    # events
    events/event.h
    events/accountdataevents.h
    events/callanswerevent.h
    events/callcandidatesevent.h
    events/callhangupevent.h
    events/callinviteevent.h
    events/directchatevent.h
    events/encryptedevent.h
    events/eventcontent.h
    events/eventloader.h
    events/reactionevent.h
    events/receiptevent.h
    events/redactionevent.h
    events/roomevent.h
    events/roomkeyevent.h
    events/roomavatarevent.h
    events/roomcreateevent.h
    events/roomcanonicalaliasevent.h
    events/roompowerlevelsevent.h
    events/roommessageevent.h
    events/roomtombstoneevent.h
    events/simplestateevents.h
    events/stateevent.h
    events/stickerevent.h
    events/typingevent.h
)

quotient_generate_headers(
  QUOTIENT_FORWARDING_HEADERS
  HEADER_NAMES
      Room,FileTransferInfo,MemberSorter
      Converters
      Connection,LoginFlows
      ConnectionData
      EncryptionManager
      EventItem,EventItemBase,EventStatus,TimelineItem
      JoinState
      Avatar
      Logging
      SyncData,RoomSummary,SyncRoomData
      SsoSession
      Settings
      Uri
      User
      UriResolver,UriResolverBase,UriDispatcher,
      Util
      Jobs/BaseJob
      Jobs/SyncJob
      Jobs/RequestData
      Jobs/MediaThumbnailJob
      Jobs/DownloadFileJob
      Events/Event
      Events/AccountDataEvents
      Events/CallAnswerEvent
      Events/CallCandidatesEvent
      Events/CallHangupEvent
      Events/CallInviteEvent
      Events/DirectChatEvent
      Events/EncryptedEvent
      Events/EventContent
      Events/EventLoader
      Events/ReactionEvent
      Events/ReceiptEvent
      Events/RedactionEvent
      Events/RoomEvent
      Events/RoomKeyEvent
      Events/RoomAvatarEvent
      Events/RoomCreateEvent
      Events/RoomCanonicalAliasEvent
      Events/RoomPowerLevelsEvent
      Events/RoomMessageEvent
      Events/RoomTombstoneEvent
      Events/SimpleStateEvents
      Events/StateEvent
      Events/StickerEvent
      Events/TypingEvent
  PREFIX Quotient
  REQUIRED_HEADERS Quotient_HEADERS
)
install(FILES ${QUOTIENT_FORWARDING_HEADERS}
      DESTINATION ${CMAKE_INSTALL_PREFIX}/include/Quotient
      COMPONENT Devel)

install(FILES ${Quotient_HEADERS}
      DESTINATION ${CMAKE_INSTALL_PREFIX}/include/quotient
      COMPONENT Devel)

