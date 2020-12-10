# Fluffelwatch

Fluffelwatch is a small livesplitter application for Linux (and potential Mac) with real and ingame timer as well as autosplit capabilities. 

# Design

On its own, Fluffelwatch provides not more than a simple timer with a manual splitter only. You need to feed it from an external program (therefore alled fluffelfood) with information when and how to autosplit or when to stop the ingame timer. These external programs can be written in any language.

This design was chosen due to the requirements on a Linux system when accessing memory of another program and to avoid the ineccessity of running a GUI application with superuser rights. With this approach, only the respective (minimalistic) fluffelfood application has to be run with superuser rights. See the Alien: Isolation fluffelfood scripts as an example.



