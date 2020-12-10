# /fluffelfood

This directory contains all extension programs (called Fluffelfood) that communicate with the respective games to provide autosplitting features, etc. In general, these programs access memory of other programs/games and, thus, need to be run as superuser.

# Communication with Fluffelwatch

Fluffelfood programs can be written in any language. They simply communicate with Fluffelwatch through a local socket (usually at /tmp/fluffelwatch) and by sending the following package:

| Type    | Length | Description                | 
|---------|--------|----------------------------|
| Byte    | 1      | Control byte for the timer |
| Integer | 4      | Current section number     |
| Integer | 4      | Icon bits                  |

A Python-based example for Alien: Isolation is provided that allows autosplitting for No Major Glitches runs. Check it out!
