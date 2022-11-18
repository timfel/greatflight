# The Great Flight

The king lies dead, and his once great armies were flushed away by a deluge of orcish warriors.
While the scattered human forces attempt to evacuate the people to the shores were the navy is
holding out, the brute force of the agressors turns inward and old hatreds resurface without an
external enemy to unite the orcs...

## Development

This game is being developed using GrafX2, ACE, and Bartman's Amiga toolchain in VSCode.
The CMake toolchain files and ACE are submodules, make sure to check them out.
ACE tools need to be compiled on the host, so open deps/ACE/tools separately and build with CMake.
Bartman's toolchain is used from the extension directly, so install that.
The project-local CMake kits are used to configure Bartman's toolchain. Choose the Win32 kit on
Windows, the Unix kit anywhere else, then build via CMake and debug via Bartman's extension should
just work.
