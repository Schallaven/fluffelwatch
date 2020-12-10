#!/usr/bin/env python3

# MIT License
#
# Copyright (c) 2020 Sven Kochmann
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# This library contains functions and logic for sending Fluffelwatch data to 
# autosplit, control the ingame time, etc.

import socket
import struct
import sys

# Default location of the Fluffelwatch socket
DEFAULT_SOCKET = "/tmp/fluffelwatch"

# Options for the control of the timer
CONTROL_NONE = 0            # Ingame timer should run normally (or continue if paused)
CONTROL_CONTINUE = 0        # Same as above; just having this extra constant to be more clear in pause/continue sequences
CONTROL_PAUSE = 1           # Ingame timer should be paused (usually done during loading times, etc.)

CONTROL_ONETIMEONLY = 200   # All control values with this value or higher will be not preserved after sending
CONTROL_START = 200         # Both timers should be reset AND started (usually done at the very beginning of a run)
CONTROL_STOP = 201          # Both timers should be stopped (usually done at the very end of a run)

# This class encapsulates the logic behind connecting to Fluffelwatch and sending
# data and the like. Keeps also track of the things sent so you can just simply
# update the icons without restating section number or control
class Fluffelwatch:
    # Socket to communicate with Fluffelwatch
    __sock__ = None

    # Internal tracking of data sent to Fluffelwatch
    __control__ = 0
    __section__ = 0
    __iconstate__ = 0

    # Error states
    error = (None, None, None)

    # Connecting to Fluffelwatch is relative easy, just open the socket as UNIX-like
    # stream socket.
    def connect(self, socketpath: str = DEFAULT_SOCKET) -> bool:
        try:
            self.__sock__ = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self.__sock__.connect(socketpath)            
        except:
            self.error = sys.exc_info()
            return False
        else:
            return True


    # Disconnecting is easy, too! Just close the socket
    def disconnect(self) -> None:
        if self.__sock__ is None:
            return

        self.__sock__.close()
        self.__sock__ = None


    # This function sends data to the specified Fluffelwatch socket. Packs it up and
    # everything, too! Data needs to be packed like this:
    # 1 Bytes: control timer
    # 4 Bytes: section number
    # 4 Bytes: icon states (bits set for max 32 icons)
    def send(self, control: int, section: int, iconstate: int) -> None:
        self.__control__ = control
        self.__section__ = section
        self.__iconstate__ = iconstate

        data = struct.pack("<BII", self.__control__, self.__section__, self.__iconstate__)
        #self.__sock__.sendall(data) 
        # Instead of using the socket functions directly, we create a file object here, write
        # to it in binary mode ("wb"), and flush it - this way, the packages a flushed into
        # the local UNIX socket and not buffered as they would be with the socket functions.
        # Closing the file object does not close the socket!    
        f = self.__sock__.makefile("wb")
        f.write(data)
        f.flush()
        f.close()

        if (self.__control__ >= CONTROL_ONETIMEONLY):
            self.__control__ = 0


    # Sends the current saved state (after updating with the functions below) to
    # Fluffelwatch. Note that you cannot send too many requests at once.
    def send_current_state(self) -> None:
        self.send(self.__control__, self.__section__, self.__iconstate__)


    # Clears the current state and sets everything to zero
    def clear_state(self) -> None:
        self.__control__ = 0
        self.__section__ = 0
        self.__iconstate__ = 0


    # Update just timer control
    def update_control(self, control: int) -> None:
        self.__control__ = control


    # Update just the section number
    def update_section(self, section: int) -> None:
        self.__section__ = section


    # Update just the pure icon state (bit-wise integer)
    def update_iconstate(self, iconstate: int) -> None:
        self.__iconstate__ = iconstate


    # Clears all icons
    def clear_iconstate(self) -> None:
        self.__iconstate__ = 0


    # Update the icons with a list of integers that correspond to the integers
    # in the respective Fluffelfood.conf file (1-based index!)
    def update_icons(self, iconlist: list) -> None:
        iconstate = 0
        for icon in iconlist:
            iconstate |= self.get_iconstate(icon)
           
        self.__iconstate__ = iconstate


    # Converts an 1-based icon index into the respective integer to send
    # to Fluffelwatch
    def get_iconstate(self, icon: int) -> int:
        return (1 << (icon-1))


    
    




