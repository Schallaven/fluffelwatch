#!/usr/bin/env python3

# MIT License
#
# Copyright (c) 2020 Sven Kochmann (Schallaven)
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

# This program controls the autosplitting of Fluffelwatch for the No-Major-Glitches
# category of Alien Isolation (Novice and Nightmare).

# Import the logic/functions for the communication with Alien: Isolation and
# Fluffelwatch. The fluffelwatch.py file is actually in the parent directory
# so it is wise to add a (symbolic) link to it to the current directory so
# that Python does not complain about the import (relative imports do not 
# work in this case). Still do not why Python cannot do simple imports from
# any file, but yeah that's how you do it.
import alieniso
import fluffelwatch

# Other imports
import argparse
import copy
from fabulous import color
from fabulous import term
import struct
import sys
import time

# Definitions
processID = 0
#binary = "~/.steam/steam/steamapps/common/Alien Isolation/bin/AlienIsolation"
binary = "/srv/steamfast/steamapps/common/Alien Isolation/bin/AlienIsolation"


# Start of the program: parsing command line
# ---------------------------------------
parser = argparse.ArgumentParser(description='Autosplitter for Fluffelwatch and Alien: Isolation - NMG.')

parser.add_argument('--pid', type=int, help='Process ID of Alien Isolation (main program, not the bash script!)')

args = parser.parse_args()

if args.pid is not None:
    processID = args.pid


# Create an object of the AlienIso class. Traditionally, we call this "Fluffel"
# after the name of the alien in Schallaven's streams.
# ---------------------------------------
fluffel = alieniso.alieniso()

# Look for binary if the process was not given by command line
if processID == 0:
    print("Looking for Alien: Isolation process...", end='')
    processID = fluffel.lookfor_processid(binary)

    if processID > 0:
        print(color.bold(color.green("OK.")))
    else:
        print(color.bold(color.red("FAILED.")))
        print("Alien: Isolation is not running or cannot be found by 'ps'. Please specify the process ID with --pid.")
        sys.exit(-1)

else:
    print("Process ID for Alien: Isolation was specified as %d." % processID)


# Connect to the Alien: Isolation process
# ---------------------------------------
print("Connecting to Alien: Isolation process...", end='')
ret = fluffel.connect()

if ret == False:
    print(color.bold(color.red("Error while opening the memory file of process %d: " % processID)) +
              color.red(fluffel.error[1]))
    #sys.exit(-1)
else:
    print(color.bold(color.green("OK.")))

print("Initialize memory addresses...", end='')
fluffel.init_addresses()

if len(fluffel.addresses) == 0:
    print(color.bold(color.red("Error while creating list: ")) + color.red(fluffel.error[1]))
    #sys.exit(-1)
else:
    print("\n\t", end='')
    print(color.bold(color.green("\n\t".join([alieniso.display_mem_address(x, 16) for x in fluffel.addresses]))))


# Connect to the Fluffelwatch process
# ---------------------------------------
print("Connecting to Fluffelwatch...", end='')
watch = fluffelwatch.Fluffelwatch()
ret = watch.connect()

if ret == False:
    print(color.bold(color.red("Error while opening socket '%s': " % fluffelwatch.DEFAULT_SOCKET )) +
              color.red(watch.error[1]))
    #sys.exit(-1)
else:
    print(color.bold(color.green("OK.")))


# Main program, reading data, interpreting it, and sending it to Fluffelwatch
# ---------------------------------------
print("Starting autosplitter... Use Ctrl+C to exit.\n")

# Create two objects, one for the old and one for the new state (old state is only updated if
# newstate is different from it)
oldstate = alieniso.state()
newstate = alieniso.state()
laststatustext = ""
runfinished = False

# Open a logging file for recording data from the "first start" to the "final stop"
logfile = open("alieniso_nmg.log", "w")
start = None

# Print 7 lines - one that stays as a separator and three others
# are deleted right away due to code in the while-loop.
print("\n" * 7, end='')

try:
    while not runfinished:
        # Delete three lines (fabulous did not implement them yet :/)
        for _ in range(7):
            sys.stdout.write("\033[F") # back to previous line 
            sys.stdout.write("\033[K") # clear line         

        # These two should be kept track of
        newstate.started = oldstate.started
        newstate.final = oldstate.final
        newstate.loading = oldstate.loading

        # Line 1: Gamestate            
        print("Gamestate:\t", end='')
        newstate.gamestate = fluffel.get_gamestate()
        print(alieniso.display_gamestate(newstate.gamestate))
    
        # Line 2: Loading icon            
        print("Loading icon:\t", end='')
        newstate.loading = fluffel.get_loadingicon()
        print(alieniso.display_loadingicon(newstate.loading))

        # Line 3: Fade state
        print("Fading:\t\t", end='')
        newstate.fadestate = fluffel.get_fadestate()
        newstate.fadevalue = fluffel.get_fadevalue()
        print(alieniso.display_fading(newstate.fadestate, newstate.fadevalue))

        # Line 4: Game flow
        print("Game flow:\t", end='')
        newstate.gameflow = fluffel.get_gameflow()
        print(alieniso.display_gameflow(newstate.gameflow))

        # Line 5: Level manager
        print("Level manager:\t", end='')
        newstate.levelman = fluffel.get_levelman()
        print(alieniso.display_levelman(newstate.levelman))

        # Line 6: Mission number
        print("Mission:\t", end='')
        newstate.mission = fluffel.get_mission()
        print(alieniso.display_mission(newstate.mission))        

        # Line 7: Status
        print("Status:\t\t", end='')

        # Check if everything is the same
        if newstate != oldstate:
            # Decide which (general) icons to show 
            icons = [alieniso.ICON_LOGO]

            if newstate.gamestate & alieniso.GAMESTATE_CINNEMATIC:
                icons.append(alieniso.ICON_CINEMATIC)

            if newstate.gamestate & alieniso.GAMESTATE_LOADING_SAVE:
                icons.append(alieniso.ICON_SAVEGAME)

            if newstate.gamestate & alieniso.GAMESTATE_DEAD:
                icons.append(alieniso.ICON_DEAD)

            if newstate.gamestate & alieniso.GAMESTATE_MENU or newstate.gamestate & alieniso.GAMESTATE_MAINMENU:
                icons.append(alieniso.ICON_MENU)

            if newstate.loading:
                icons.append(alieniso.ICON_LOADING)

            # Set mission number as section number
            watch.update_section(newstate.mission)            

            # Most of the following logic is based on the ASL files from Cliffs666 and fatalis:
            # https://github.com/Cliffs666/ASL/blob/master/Alien_Isolation_Auto.asl
            # https://github.com/fatalis/ASL/blob/master/alien_isolation.asl         

            # Check if the run started. This is usually at the beginning of mission 1
            if not newstate.started and newstate.mission == 1 and \
                 ((newstate.fadestate == alieniso.FADESTATE_FADEING and newstate.fadevalue > 0.0) or \
                  (oldstate.gameflow == alieniso.GAMEFLOW_LOADSCREEN and newstate.gameflow == alieniso.GAMEFLOW_INGAME)):
                laststatustext = "Start!"
                start = time.time()
                newstate.started = True
                watch.update_control(fluffelwatch.CONTROL_START)
                time.sleep(0.1)

            # Check if loading started
            elif (oldstate.levelman == alieniso.LEVELMAN_LOADED and newstate.levelman == alieniso.LEVELMAN_LOADING_START) or \
                newstate.gameflow == alieniso.GAMEFLOW_LOADSCREEN:
                laststatustext = "Loading!"
                newstate.loading = True
                watch.update_control(fluffelwatch.CONTROL_PAUSE)                

            # Check if loading stopped
            elif newstate.fadestate == alieniso.FADESTATE_FADEING and oldstate.fadevalue < 0.2 and newstate.fadevalue > 0.2:
                laststatustext = "Stopped loading!"
                newstate.loading = False
                watch.update_control(fluffelwatch.CONTROL_CONTINUE)

            # This is the initial fading in sequence in mission 19. Mark it that it happend (so any blackout here and just before the start
            # of this mission do not interfere)
            elif (newstate.mission == 19 and newstate.fadestate == alieniso.FADESTATE_FADEING and oldstate.fadevalue < 0.5 and newstate.fadevalue > 0.5):
                laststatustext = "Final mission!"
                newstate.final = True

            # Final mission and blackout again?
            elif newstate.final and newstate.mission == 19 and \
                 newstate.fadestate == alieniso.FADESTATE_BLACK and newstate.gameflow == alieniso.GAMEFLOW_INGAME:
                laststatustext = "Final stop!"    
                watch.update_control(fluffelwatch.CONTROL_STOP)  
                runfinished = True          

            # Mission changed
            elif oldstate.mission > 0 and oldstate.mission < newstate.mission:
                laststatustext = "Split mission"

            # Send new data to Fluffelwatch
            watch.update_icons(icons)
            watch.send_current_state()

            # Copy the newstate into the old one
            oldstate = copy.deepcopy(newstate)

            if newstate.started and len(laststatustext) > 0:
                logfile.write(time.ctime())
                logfile.write("\t%.3f s\t%s\n" % ((time.time() - start), laststatustext))
                logfile.flush()

        print(laststatustext)

        # Sleep for some milliseconds to not burn CPU cycles
        # Set this to a quarter of the resolution you want to have, e.g.
        # if you want 100ms resolution, set this to 25ms
        time.sleep(0.025) # time in seconds, i.e. 0.025 = 25 ms 


except KeyboardInterrupt:
    # The terminal will print "^C" and the like for keyboard
    # interrupts, so just print a single line (with end of
    # line here) to get a clean new line start.
    print("")
    pass


# Cleanup
# ---------------------------------------
logfile.close()
fluffel.disconnect()
watch.disconnect()


