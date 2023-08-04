# Tether installer

The Tether installer is an auto updater and mod manager for the
[Northstar modding framework for Titanfall 2](https://github.com/R2Northstar/Northstar).
It is made using [KlemmUI](https://github.com/Klemmbaustein/KLemmUI)
and is avaliable for Windows.

## Download

You can find pre-packaged releases of the installer [here](https://github.com/Klemmbaustein/TetherInstaller/releases/tag/v1.1.1).

## Build from source

First, clone the repository with submodules. `git clone --recurse-submodules https://github.com/Klemmbaustein/TetherInstaller.git`

### Windows MSVC:

1. Run `Setup.ps1` with the visual studio developer powershell.
2. Build the solution.

### Linux GNU Makefile:

1. Install cURL with SSH on your system.
2. Follow [The steps detailed here](https://github.com/Klemmbaustein/KlemmUI#readme) to install the KlemmUI library on your system.
3. Run `make` in the `NorthstarInstaller` directory.

## FAQ

Q: Why?

A: I don't know.

----------------------------------

Q: Why would I use this over [VTOL](https://github.com/R2NorthstarTools/VTOL)
or [Flightcore](https://github.com/R2NorthstarTools/FlightCore/)?

A: You shouldn't.
