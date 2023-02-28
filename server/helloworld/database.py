"""
The "database" module that runs this application.
"""

from dataclasses import dataclass
from datetime import datetime
from typing import List, Dict, Optional, Callable, Union


class UserDevice:
    def __init__(self, mac: str, username: Optional[str] = None) -> None:
        self._mac = mac
        self._username = mac if username == None else username
        self._unread_messages = list()
    
    def mac_address(self) -> str:
        """Returns the MAC address of the device."""
        return self._mac
    
    def username(self) -> str:
        """Returns the username of the device."""
        return self._username
    
    def set_username(self, new_username: str) -> str:
        """Sets the username of the device."""
        self._username = new_username
    
    def add_pending_message(self, message: str, sender: str) -> None:
        """Adds the message to the list of messages that the user device has not
        read yet.
        """
        # Sender is gauranteed to be in the database so no need to check
        self._unread_messages.append(Message(sender, message, datetime.now()))
    
    def get_pending_messages(self, limit: int = 1) -> List['Message']:
        """Returns a list of messages to read. This removes the messages from
        the user since they have been read.
        """
        result = []
        while limit > 0 and len(self._unread_messages) > 0:
            result.append(self._unread_messages.pop(0))
            limit -= 1
        return result

    def count_pending_messages(self) -> int:
        """Returns the number of messages not read yet."""
        return len(self._unread_messages)
    
    def as_dict(self) -> Dict[str, Union[str, List[str]]]:
        return {
            'macAddress': self._mac,
            'username': self._username,
            'unreadMessages': [msg.as_dict() for msg in self._unread_messages]
        }


@dataclass
class Message:
    sender_mac: str
    content: str
    timestamp: datetime

    def as_dict(self) -> Dict[str, str]:
        return {
            'macAddress': self.sender_mac,
            'content': self.content,
            'time': self.timestamp.strftime('%Y-%m-%d %I:%M:%S %p')
        }



# key = mac address, value = user data
DATABASE: Dict[str, UserDevice] = dict()


class DatabaseException(Exception):
    def __init__(self, message: str) -> None:
        super().__init__(message)
        self._message = message
    
    def get_messsage(self) -> str:
        return self._message


def add_user(mac_address: str, username: str = None) -> None:
    """Adds a user to the database if it does not exist. No action is taken if
    it exists.
    """
    if contains_user(mac_address):
        return
    DATABASE[mac_address] = UserDevice(mac_address, username)


def remove_user(mac_address: str) -> None:
    """Removes a user from the database deleting all of its associated data. If
    the user does not exist, no action is taken.
    """
    if not contains_user(mac_address):
        return
    del DATABASE[mac_address]


def contains_user(mac_address: str) -> bool:
    """Determines if a user with the same MAC address exists in the database."""
    return mac_address in DATABASE


def get_user(mac_address: str) -> UserDevice:
    """Returns the user with the MAC address specified. If no user exists, then
    an exception is thrown.
    """
    if not contains_user(mac_address):
        message = f'Could not find user with mac address "{mac_address}"'
        raise DatabaseException(message)
    return DATABASE[mac_address]


def for_each_user(func: Callable[[UserDevice], None]) -> None:
    """Applies a function every user in the database."""
    for user in DATABASE.values():
        func(user)


def to_dict():
    """Converts the database to a dictionary for viewing purposes."""
    result = dict()
    for key, value in DATABASE.items():
        result[key] = value.as_dict()
    return result