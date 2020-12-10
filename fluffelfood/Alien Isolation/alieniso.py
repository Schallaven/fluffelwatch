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

# This library contains functions and logic for querying the status of a running
# Alien Isolation (Linux version, ELF binary) instance.

# Imports
from fabulous import color
from fabulous import term
import os
import struct
import subprocess
import sys

# Icon definitions (these should be same as defined in the .conf file); 1-based indices!
ICON_LOGO = 1
ICON_LOADING = 7
ICON_SAVEGAME = 8
ICON_MENU = 9
ICON_CINEMATIC = 11
ICON_DEAD = 12

# Gamestate flags
# Not listed gamestates are unknown (i.e. 4, 32, 64, ...)
GAMESTATE_NONE = 0              # Normal game state
GAMESTATE_PAUSED = 1            # Game is paused
GAMESTATE_DEAD = 2              # Main actor (usually Ripley) is dead
GAMESTATE_LOADING_SAVE = 8      # Loading a savegame
GAMESTATE_CINNEMATIC = 16       # Cinnematic is playing
GAMESTATE_MENU = 1024           # Menu is shown 
GAMESTATE_MAINMENU = 32768      # Mainmenu is shown (the one with the gas giant in the background)

# Loading icon area status
# Other values are unknown (and not observed)
LOADICON_NONE = 0               # Nothing is shown in the bottom right corner
LOADICON_SKIPBAR = 1            # Skip cinnematic bar is shown and filled (i.e user presses enter)
LOADICON_LOADING = 256          # Loading icon animation is shown

# Gameflow state
# Other values are unknown
GAMEFLOW_NOTHING = 0            # No flow :(
GAMEFLOW_INIT = 1               # Initialize Alien: Isolation (state before the intro plays)
GAMEFLOW_MAINMENU = 2           # Mainmenu
GAMEFLOW_INGAME = 4             # Ingame
GAMEFLOW_CREDITS = 5            # Credits are shown
GAMEFLOW_LOADSCREEN = 6         # Loading screen is shown

# Level manager state
# Other values are unknown
LEVELMAN_NOTHING = 0            # Doesn't do anything
LEVELMAN_LOADING = 3            # Loading level data
LEVELMAN_LOADED = 5             # Level is loaded
LEVELMAN_LOADING_START = 7      # Wrapping up old level state and start loading new level

# Fading state
# These values define how the fade factor is interpreted or applied
FADESTATE_NONE = 0              # No fading/overlay effect, full visual (fade factor is ignored)
FADESTATE_BLACK = 1             # Black screen (fade factor is ignored)
FADESTATE_FADEING = 2           # Screen fades in/out (fade factor defines how much is visible)
FADESTATE_FADEBLINK = 3         # Eye blink effect/overlay (darkens from above of the screen defined by the fade factor)

# Memory addresses (received and tested with AlienIsolation ELF LSB binary, CRC32 = 839a6c9a, size = 60,460,400 Bytes
# These are pointer lists, e.g. the value at the first address will be read, then the second value will be added, a
# new address will be read, third value will be added, etc.
# MEMORY_ADDRESS = [initial, add1, add2, add3, ...]

# For fixed addresses only one entry is required. 
# MEMORY_ADDRESS = [fixed]

# If a memory address just needs to be read at a specific other address, simply add a zero, e.g.
# MEMORY_ADDRESS = [initial, 0]

# ELF64 binaries are always loaded at 0x4000000. So, all these addresses are a little bit larger than the ones 
# on Windows. Furthermore, since the binaries themselves are modified, there is no simple way of just adding a
# constant to the address of the Windows binary. All the following addresses have been found with PINCE/gdb.

MEMORY_GAMESTATE = [0x4024f40, 0]           # Gamestate is saved here
MEMORY_LOADINGICON = [0x4088510, 0x1c]      # Loading icon state
MEMORY_FADESTATE = [0x415df88]              # Fade state
MEMORY_FADENUM = [0x415df8C]                # Fade value (float)
MEMORY_GAMEFLOW = [0x4024f60, 0x90, 0x10]   # Game flow state
MEMORY_LEVELMAN = [0x4024f60, 0x78, 0x90]   # Level manager state
MEMORY_MISSION = [0x47899a0, 0x560, 0xe0]   # Current mission number (changes when level/mission is completely loaded)

# There are some other memory addressed, which are not used (yet). But since we know them, why throw away the knowledge?
MEMORY_GAMESTATE_OLD = [0x4024f44, 0]       # Previous(?) gamestate is saved here (just 4 bytes behind the first gamestate!)
MEMORY_PAUSE = [0x383f571]                  # Is the game processes paused (e.g. when player is on the map)?
MEMORY_LASTSAVE = [0x402896f]               # This is usually the mission number or zero if the last save was done at a station


# Helper function to format an address (blue, bold)
def display_mem_address(address: int, length: int = 8) -> str:
    format_str = "0x%0" + str(length) + "X"
    address_str = color.blue(color.bold(format_str % address))
    return str(address_str)


# Simple function that parses the mission number. Just returns a string with color basically.
def display_mission(mission: int) -> str:
    return color.bold(color.green("M%02d" % mission))    


# Simple function that parses the game state. Just returns a string with color basically.
def display_gamestate(gamestate: int) -> str:
    states = []
    if gamestate & GAMESTATE_PAUSED:
        states.append(str(color.blue("paused")))
        gamestate -= GAMESTATE_PAUSED
    if gamestate & GAMESTATE_DEAD:
        states.append(str(color.red("dead")))
        gamestate -= GAMESTATE_DEAD
    if gamestate & GAMESTATE_LOADING_SAVE:
        states.append(str(color.yellow("loading savegame")))
        gamestate -= GAMESTATE_LOADING_SAVE
    if gamestate & GAMESTATE_CINNEMATIC:
        states.append(str(color.magenta("cinnematic playing")))
        gamestate -= GAMESTATE_CINNEMATIC
    if gamestate & GAMESTATE_MENU:
        states.append(str(color.cyan("menu")))
        gamestate -= GAMESTATE_MENU
    if gamestate & GAMESTATE_MAINMENU:
        states.append(str(color.cyan("main menu")))
        gamestate -= GAMESTATE_MAINMENU
    if (gamestate > 0):
        states.append(str("unknown rest: %d" % gamestate))

    return color.bold(", ".join(states))


# Simple function that parses the loading icon state. Just returns a string with color basically.
def display_loadingicon(loadingicon: int) -> str:
    load = ""
    if loadingicon == LOADICON_LOADING:
        load = color.yellow("visible")
    elif loadingicon == LOADICON_SKIPBAR:
        load = color.blue("pressing enter")
    elif loadingicon == LOADICON_NONE:
        load = color.black("hidden")
    else:
        load = color.cyan("unknown (%d)" % loadingicon)
        
    return color.bold(load)


# Simple function that parses the game flow state. Just returns a string with color basically.
def display_gameflow(gameflow: int) -> str:
    flow = ""
    if gameflow == GAMEFLOW_INIT:
        flow = color.yellow("initializing")
    elif gameflow == GAMEFLOW_MAINMENU:
        flow = color.yellow("main menu")
    elif gameflow == GAMEFLOW_INGAME:
        flow = color.blue("ingame")
    elif gameflow == GAMEFLOW_CREDITS:
        flow = color.red("credits")
    elif gameflow == GAMEFLOW_LOADSCREEN:
        flow = color.magenta("loading screen")
    elif gameflow == GAMEFLOW_NOTHING:
        flow = color.black("sleeping")
    else:
        flow = color.cyan("unknown (%d)" % gameflow)
        
    return color.bold(flow)


# Simple function that parses the state of the levelmanager. Just returns a string with color basically.
def display_levelman(levelmanstate: int) -> str:
    state = ""
    if levelmanstate == LEVELMAN_LOADING:
        state = color.yellow("loading")
    elif levelmanstate == LEVELMAN_LOADED:
        state = color.green("loaded")
    elif levelmanstate == LEVELMAN_LOADING_START:
        state = color.red("start loading")
    elif levelmanstate == LEVELMAN_NOTHING:
        state = color.black("sleeping")
    else:
        state = color.cyan("unknown (%d)" % levelmanstate)
        
    return color.bold(state)


# Simple function that parses the fading state. Just returns a string with color basically.
def display_fading(fadestate: int, fadenum: float) -> str:
    state = ""
    if fadestate == FADESTATE_NONE:
        state = color.black("none")
    elif fadestate == FADESTATE_BLACK:
        state = color.blue("black screen")
    elif fadestate == FADESTATE_FADEING:
        state = color.black("fading in/out (%.1f)" % fadenum)
    elif fadestate == FADESTATE_FADEBLINK:
        state = color.black("blink effect (%.1f)" % fadenum)
    else:
        state = color.cyan("unknown (%d, %.1f)" % (fadestate, fadenum))
        
    return color.bold(state)


# This class encapsulates the logic behind finding the Alien Isolation binary as well as connecting and
# querying the values. Please note that this requires superuser rights on most systems.
class alieniso:
    # Process ID of Alien: Isolation
    __processid__ = 0

    # Memory file used for reading data
    __memfile__ = None

    # Error
    error = (None, None, None)

    # Internal indices used for addresses, etc
    GAMESTATE = 0
    LOADINGICON = 1
    FADESTATE = 2
    FADENUM = 3
    GAMEFLOW = 4
    LEVELMAN = 5
    MISSION = 6
    
    MEMORY_LIST = [MEMORY_GAMESTATE, MEMORY_LOADINGICON, MEMORY_FADESTATE, MEMORY_FADENUM, MEMORY_GAMEFLOW, MEMORY_LEVELMAN, MEMORY_MISSION]

    # Addresses
    addresses = []

    # Getter/setter for process id
    def get_processid(self) -> int:
        return self.__processid__

    def set_processid(self, processid: int) -> None:
        self.__processid__ = processid

    # Look for the Alien: Isolation binary with "ps". Returns the 
    # process id if found otherwise zero
    def lookfor_processid(self, binary: str) -> int:
        # Expand user directory if necessary
        binary = os.path.expanduser(binary)

        # Run "ps -eo pid,command" that will give as a nice list looking like
        # 11111 command -params        
        # We split these lines at space character with a maximum of 1 split. This
        # way we have the process id in the first item and the whole command line
        # in the second item (which will be compared to binary).
        pscmd = subprocess.Popen('ps -eo pid,command', shell=True, stdout=subprocess.PIPE)
        for line in pscmd.stdout:
            items = line.decode().strip().split(' ', 1)
            
            if len(items) != 2:
                continue

            if items[1].startswith(binary):
                self.__processid__ = int(items[0])
                return self.__processid__

        return 0


    # Connect to the Alien: Isolation process memory file
    def connect(self) -> bool:
        if self.__processid__ == 0:
            self.error = (None, "No process id set", None)
            return False

        try:
            self.__memfile__ = open("/proc/" + str(self.__processid__) + "/mem", 'rb', 0)
        except:
            self.error = sys.exc_info()
            return False
        else:
            return True
    

    # Disconnect from Alien: Isolation. Also sets process ID to zero.
    def disconnect(self) -> None:
        self.__processid = 0
        self.__memfile__.close()

    
    # Read a specific memory address and return its contents as an integer
    def read_address(self, address: int, length: int) -> int:
        if self.__memfile__ is None:
            return 0

        try:
            self.__memfile__.seek(address)  
            chunk = self.__memfile__.read(length)    
            # Need to find the right integer string for whatever length is given
            format = ""
            if length in [0, 3, 5, 6, 7] or length > 8:
                return 0
            else:
                format = ["", "B", "H", "", "I", "", "", "", "Q"][length]
        
        except:
            error = sys.exc_info()
            return 0
        else:
            return int(struct.unpack("<" + format, chunk)[0])


    # Read a specific memory address and return its contents as a float (4 bytes)
    def read_address_float(self, address: int) -> float:
        if self.__memfile__ is None:
            return 0

        try:
            self.__memfile__.seek(address)  
            chunk = self.__memfile__.read(4)            
        except:
            error = sys.exc_info()
            return 0
        else:
            return float(struct.unpack("<f", chunk)[0])


    # Initialize all memory addresses used
    def init_addresses(self) -> None:
        self.addresses = []
        # Go through the MEMORY_LIST and handle each case more or less individually
        for pointerlist in self.MEMORY_LIST:
            # No list? Then simply save a null pointer here
            if len(pointerlist) == 0:
                self.addresses.append(0)
                continue

            # One item? Simply save it
            if len(pointerlist) == 1:
                self.addresses.append(pointerlist[0])
                continue

            # Start with the first address in the list            
            current_address = pointerlist[0]

            # Step by step: read the address (always 8 byte length), add the next
            # shift, continue
            for shift in pointerlist[1:]:                
                current_address = self.read_address(current_address, 8)
                current_address += shift

            # Save the final address
            self.addresses.append(current_address)


    # Read gamestate (4 bytes)
    def get_gamestate(self) -> int:
        return self.read_address(self.addresses[self.GAMESTATE], 4)


    # Read loading icon (2 bytes)
    def get_loadingicon(self) -> int:
        return self.read_address(self.addresses[self.LOADINGICON], 2)


    # Read fade state (4 bytes)
    def get_fadestate(self) -> int:
        return self.read_address(self.addresses[self.FADESTATE], 4)


    # Read fade value (float, 4 bytes)
    def get_fadevalue(self) -> float:
        return self.read_address_float(self.addresses[self.FADENUM])

    
    # Read gameflow (4 bytes)
    def get_gameflow(self) -> int:
        return self.read_address(self.addresses[self.GAMEFLOW], 4)


    # Read level manager state (4 bytes)
    def get_levelman(self) -> int:
        return self.read_address(self.addresses[self.LEVELMAN], 4)

    
    # Read current mission (2 bytes)
    def get_mission(self) -> int:
        return self.read_address(self.addresses[self.MISSION], 2)
        

# Define a simple class to keep track of the current state
class state:
    started = False
    loading = False
    final = False

    gamestate = 0
    loadingicon = 0
    fadestate = 0
    fadevalue = 0.0
    gameflow = 0
    levelman = 0    
    mission = 0

    # Check if everything is the same
    def __eq__(self, other):
        return self.started == other.started and \
               self.loading == other.loading and \
               self.final == other.final and \
               self.gamestate == other.gamestate and \
               self.loadingicon == other.loadingicon and \
               self.fadestate == other.fadestate and \
               self.fadevalue == other.fadevalue and \
               self.gameflow == other.gameflow and \
               self.levelman == other.levelman and \
               self.mission == other.mission 


