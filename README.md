# Esper-RE
This repo hosts tools and scripts for modding and researching Phantom Dust files.
These tools mainly focus on [ALR](https://phantomdust.miraheze.org/wiki/Modding/File_Formats/ALR)
files, but there are also some simple tools for [Deck/Arsenal](https://phantomdust.miraheze.org/wiki/Modding/File_Formats/Decks)
files and [SSB](https://phantomdust.miraheze.org/wiki/File_Formats/SSB) script files. You can find more info
about the game and file formats on the [Phantom Dust Modding Wiki](https://phantomdust.miraheze.org/wiki/Modding).

## Polaris [`src/polaris`]
This is the main program for this repo, a graphical editor for ALR files. It
understands (to varying degrees) textures, meshes, animations, and skeleton
data found in ALRs. Polaris is designed to replace normal hex editors for ALR
modding, without giving up the flexibility. There are mini hex editors in
nearly every window, so you can quickly jump to the data you want (and see
only that data).

Support status for `.alr` files:

| Feature           | Viewing                    | Export                  | Import                                     |
| ----------------- | -------------------------- |------------------------ |------------------------------------------- |
| Textures          | ✅                         | ✅(`.dds`)              | ✅ (`.dds`, at same resolution & format)   |
| Meshes            | ✅                         | ✅(`.obj`)              | ❌                                         |
| Animations        | ✅ (graph)                 | ❌ (WIP `.anim` export) | ❌                                         |
| Skeleton          | ❌                         | ✅ (`.dae`)             | ❌                                         |

Polaris also supports several other Phantom Dust files:
- Stage layout (`.dat`) files can be loaded alongside their corresponding ALR, to spawn in, render, and edit dynamic objects like chairs and railings (Surface stages only, no underground stages).
- Music (`.stx`) files can be generated from `.wav`, `.mp3`, `.mod`, `.xm`, or `.s3m` files. They can also be exported to `.wav` and played back directly in Polaris
- Sound effect (`.bin`) files can be exported to `.wav`
- Quest (`questdata.qdt`) files can be edited (though they're not fully understood yet)
- `.mk` and `.ak` files are bundles of NPC models and animations stored in `.alr` and related files. Polaris can extract and create these bundles.

Here's the underground bar map rendered in Polaris:    
<img src="https://github.com/user-attachments/assets/62e2b8c2-1e83-4666-9bda-007ecec425e8"  style="width:90%;"/>

Here's Polaris animating Meister's low-poly model, while editing the quests file and playing back a custom STX file it generated from the FastTracker II song `domatron.xm`:    
<img src="https://github.com/user-attachments/assets/dab7b85a-62ed-4d1f-8103-28c9f902cdc7" style="width:70%;"/>

### Command-line Options
Polaris takes a filename as its first argument (which also means you can drag-and-drop
files onto the executable). You can also specify another flag to run a specific
operation in *headless* mode (no GUI):        
`polaris [alr filename] [flag]`

- `--dump-textures`
  - Dump all textures to a `textures` folder in DDS format.
- `--dump-materials`
  - Export materials from an ALR file in MTL format (for use with OBJ files)
  - The exported OBJ files have material assignment data, but you have to manually write `mtllib [your .mtl file]` at the start.
- `--extract-audio`
  - Export all sound effects from multiple .bin files to a folder, in WAV format
- `--validate`
  - Run assertions and consistency checks across the whole file. Doesn't guarantee
    the file will work in the game, just that nothing looks wrong based on our
    current knowledge of ALRs.

## Deck Reader [`src/deck_reader`]
This is a very simple TUI editor for deck/arsenal files. In the style of a simple batch
script menu, you select a menu option by number and type in a new value. The menu updates
in-place instead of scrolling.

<img src="https://github.com/user-attachments/assets/8992a8e4-cddf-495b-99c2-db9998c41bda" style="width:70%;"/>

## SSB Tool [`src/ssb`]
This is a barebones tool, which only prints an SSB's function export table. For better SSB tools, take a look at these repos:
- [hantu](https://github.com/VSaige3/hantu)
- [pd-ssb-decomp](https://github.com/VSaige3/pd-ssb-decomp)
For more information on the file format, check out the [wiki page](https://phantomdust.miraheze.org/wiki/File_Formats/SSB).

<img src="https://github.com/user-attachments/assets/57e7bc37-d45b-413b-92fb-4d7d091a413b" style="width:70%;"/>
