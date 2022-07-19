# Esper Reader
This is a tool written in C++ to parse files from Phantom Dust. Right now, it only supports the most basic file format, deck files. I plan to support
parsing and maybe dumping of data from many other formats, such as ALR and SSB. Parsers will all be in separate source files, with the main function
simply figuring out which parser to use. Since there's only 1 parser right now (and deck files have no file extension or magic), it just goes straight
to deck parsing. Contributions are welcome.

## Usage
Drag and drop any binary deck file onto EsperReader.exe.

## Building
If you want debugging to work, you'll need to create a `decks` folder in the project directory, then copy in `DECK_PLAYER_0` from `Assets/Data/com/deck`
in your Phantom Dust dump. This is the file that it will parse by default.