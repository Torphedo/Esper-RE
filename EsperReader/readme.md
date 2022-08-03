# Esper Reader
This is a tool written in C++ to parse files from Phantom Dust. Right now, it only supports deck files, and some basic elements of ALR. I plan to support
parsing and maybe dumping of data from SSB in the future. Parsers will all be in separate source files, with the main function
simply figuring out which parser to use. Contributions are welcome.

## Usage
Drag and drop any binary deck file or ALR file onto EsperReader.exe.

## Building
If you want debugging to work, you'll need to create a `decks` folder in the project directory, then copy in `DECK_PLAYER_0` from `Assets/Data/com/deck`
in your Phantom Dust dump. This is the file that it will parse by default.
