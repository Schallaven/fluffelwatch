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

# This program tests the communication with Fluffelwatch by starting the timers,
# pausing/continuing the ingame timer, stopping the timer.

import fluffelwatch
import sys
import time

# Create a fluffelwatch object and connect
watch = fluffelwatch.Fluffelwatch()

print("Connecting to Fluffelwatch...", end='')
if not watch.connect():
    print("FAILED.", watch.error[1])
    sys.exit(-1)

print("OK.")

# Starting sequence here
print("Starting sequence...")

print("\t", time.ctime(), "Starting timer.")
watch.update_control(fluffelwatch.CONTROL_START)
watch.update_section(1)
watch.send_current_state()
print("\t", time.ctime(), "Waiting 10 seconds.")
time.sleep(10)

print("\t", time.ctime(), "Showing icon 1, 7, and 8.")
watch.update_icons([1, 7, 8])
watch.send_current_state()
print("\t", time.ctime(), "Waiting 5 seconds.")
time.sleep(5)

# Pausing and continuing several times
for i in range(3):
    waiting = (i+1) * 3

    print("")
    print("\t", time.ctime(), "Pausing ingame timer. Showing icon 1 and 7.")
    watch.update_icons([1, 7])
    watch.update_control(fluffelwatch.CONTROL_PAUSE)
    watch.send_current_state()
    print("\t", time.ctime(), "Sleeping for %d seconds" % (waiting * 2))
    time.sleep(waiting * 2)

    print("\t", time.ctime(), "Continuing ingame timer. Showing only icon 1.")
    watch.update_icons([1])
    watch.update_control(fluffelwatch.CONTROL_CONTINUE)
    watch.send_current_state()
    print("\t", time.ctime(), "Sleeping for %d seconds" % (waiting))
    time.sleep(waiting)

    print("\t", time.ctime(), "Autosplit to section %d" % (i+2))
    watch.update_section(i+2)
    watch.send_current_state()

print("")
print("\t", time.ctime(), "Sleeping for 3 seconds")
time.sleep(3)

print("\t", time.ctime(), "Ending run.")
watch.update_control(fluffelwatch.CONTROL_STOP)
watch.send_current_state()

time.sleep(1)
print("Ending sequence...")
watch.disconnect()

print("")




