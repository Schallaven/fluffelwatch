# Fluffelwatch

Fluffelwatch is a small livesplitter application for Linux (and potential Mac) with real and ingame timer as well as autosplit capabilities. 

# Design

On its own, Fluffelwatch provides not more than a simple timer with a manual splitter only. It needs to be _fed_ from an external program (therefore, called _fluffelfood_, see respective folder) with information when and how to autosplit or when to stop the ingame timer. These external programs can be written in any language.

This design was chosen due to the requirements on a Linux system when accessing memory of another program and to avoid the unnecessity of running a GUI application with superuser rights. With this approach, only the respective (minimalistic) fluffelfood application has to be run with superuser rights and Fluffelwatch itself can remain in user space. See the Alien: Isolation fluffelfood scripts as an example.

![grafik](https://user-images.githubusercontent.com/6428497/101819897-d6320b80-3af3-11eb-91df-9d7a4b659d80.png)

