// SPDX-FileCopyrightText: 2015 Felix Rohrbach <kde@fxrh.de>
// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "avatar.h"
#include "util.h"

#include <QtCore/QObject>

namespace Quotient {
class Connection;
class Room;
class RoomMemberEvent;

//! This class provides an interface to a given user's profile.
//!
//! \note The User class is not intended for getting the data to visualise a user
//!       in the context of a particular room. For that a Quotient::RoomMember object
//!       should be obtained from a Quotient::Room as this will account for the
//!       user setting an avatar or name that applies to that room only.
//!
//! \sa Quotient::RoomMember
class QUOTIENT_API User : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(bool isGuest READ isGuest CONSTANT)
    Q_PROPERTY(int hue READ hue CONSTANT)
    Q_PROPERTY(qreal hueF READ hueF CONSTANT)
    Q_PROPERTY(QString name READ name NOTIFY defaultNameChanged)
    Q_PROPERTY(QString displayName READ displayname NOTIFY defaultNameChanged STORED false)
    Q_PROPERTY(QString fullName READ fullName NOTIFY defaultNameChanged STORED false)
    Q_PROPERTY(QString avatarMediaId READ avatarMediaId NOTIFY defaultAvatarChanged STORED false)
    Q_PROPERTY(QUrl avatarUrl READ avatarUrl NOTIFY defaultAvatarChanged)
public:
    User(QString userId, Connection* connection);

    Connection* connection() const;

    //! \brief Get unique stable user id
    //!
    //! The Matrix user ID is generated by the server and is never changed.
    QString id() const;

    //! \brief Get the default user name.
    //!
    //! This may be empty if the user has not set one.
    //!
    //! \note If you are visualizing a user in a room context you should be using
    //!       Quotient::RoomMember->name() as that will account for the user
    //!       having a unique name in that room.
    //!
    //! \sa Quotient::RoomMember::name()
    QString name(const Room* room = nullptr) const;

    //! \brief Get the name to show on the user's profile
    //!
    //! This is intended to always give you something that can be displayed in a
    //! UI. If the user doesn't have a default name or one is not available the
    //! user's matrix ID will be used.
    //!
    //! \note If you are visualizing a user in a room context you should be using
    //!       Quotient::RoomMember->displayName() as that will account for the user
    //!       having a unique name in that room.
    //!
    //! \sa Quotient::RoomMember::displayname()
    QString displayname(const Room* room = nullptr) const;

    //! \brief Get user's profilename and id in one string
    //!
    //! This is intended to always give you something that can be displayed in a
    //! UI. If the user doesn't have a default name or one is not available the
    //! fucntion will return the user's matrix ID only.
    //!
    //! \note If you are visualizing a user in a room context you should be using
    //!       Quotient::RoomMember->fullName() as that will account for the user
    //!       having a unique name in that room.
    //!
    //! \sa Quotient::RoomMember::fullName()
    QString fullName(const Room* room = nullptr) const;

    //! \brief Whether the user is a guest
    //!
    //! As of now, the function relies on the convention used in Synapse
    //! that guests and only guests have all-numeric IDs. This may or
    //! may not work with non-Synapse servers.
    bool isGuest() const;

    [[deprecated("Quotient::RoomMember::hue() should be used instead.")]]
    int hue() const;
    [[deprecated("Quotient::RoomMember::hueF() should be used instead.")]]
    qreal hueF() const;

    [[deprecated("For visualising in a room context Quotient::Room::memberAvatar() should be used instead, otherwise use Quotient::Connection::userAvatar().")]]
    const Avatar& avatarObject(const Room* room = nullptr) const;
    [[deprecated("For visualising in a room context Quotient::Room::memberAvatar() should be used instead, otherwise use Quotient::Connection::userAvatar().")]]
    Q_INVOKABLE QImage avatar(int dimension,
                              const Quotient::Room* room = nullptr) const;
    [[deprecated("For visualising in a room context Quotient::Room::memberAvatar() should be used instead, otherwise use Quotient::Connection::userAvatar().")]]
    Q_INVOKABLE QImage avatar(int requestedWidth, int requestedHeight,
                              const Quotient::Room* room = nullptr) const;
    [[deprecated("For visualising in a room context Quotient::Room::memberAvatar() should be used instead, otherwise use Quotient::Connection::userAvatar().")]]
    QImage avatar(int width, int height, const Room* room,
                  const Avatar::get_callback_t& callback) const;

    //! \brief The default mxc URL as a string for the user avatar
    //!
    //! This can be empty if none set.
    //!
    //! \note When visualising a user in the room context use
    //!       Quotient::RoomMember::avatarMediaId() instead.
    //!
    //! \sa RoomMember
    QString avatarMediaId(const Room* room = nullptr) const;

    //! \brief The default mxc URL for the user avatar
    //!
    //! This can be empty if none set.
    //!
    //! \note When visualising a user in the room context use
    //!       Quotient::RoomMember::avatarUrl() instead.
    //!
    //! \sa RoomMember
    QUrl avatarUrl(const Room* room = nullptr) const;

public Q_SLOTS:
    //! Set a new name in the global user profile
    void rename(const QString& newName);

    //! Set a new name for the user in one room
    void rename(const QString& newName, Room* r);

    //! Upload the file and use it as an avatar
    bool setAvatar(const QString& fileName);

    //! Upload contents of the QIODevice and set that as an avatar
    bool setAvatar(QIODevice* source);

    //! Removes the avatar from the profile
    void removeAvatar();

    //! \brief Create or find a direct chat with this user
    //!
    //! The resulting chat is returned asynchronously via
    //! Connection::directChatAvailable().
    void requestDirectChat();

    //! Add the user to the ignore list
    void ignore();

    //! Remove the user from the ignore list
    void unmarkIgnore();

    //! Check whether the user is in ignore list
    bool isIgnored() const;

    //! \brief Force loading display name and avartar url
    //!
    //! This is required in some cases where the you need a user's default details
    //! independent of the room, e.g. in a profile page.
    void load();

Q_SIGNALS:
    //! The default name of the user has changed
    void defaultNameChanged();

    //! The default avatar of the user has changed
    void defaultAvatarChanged();

private:
    class Private;
    ImplPtr<Private> d;

    template <typename SourceT>
    bool doSetAvatar(SourceT&& source);
};
} // namespace Quotient
