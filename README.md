# Esper-RE
Tools, templates, and information for modding and researching Phantom Dust files.
These tools mainly focus on [ALR](https://phantomdust.miraheze.org/wiki/Modding/File_Formats/ALR)
files, but there are also some simple tools for [Deck/Arsenal](https://phantomdust.miraheze.org/wiki/Modding/File_Formats/Decks)
files and [SSB](https://phantomdust.miraheze.org/wiki/File_Formats/SSB) menu script files. You can find more info
about the game and file formats on the [Phantom Dust Modding Wiki](https://phantomdust.miraheze.org/wiki/Modding).

## Polaris [`src/polaris`]
This is the main program for this repo, a graphical editor for ALR files. It
understands (to varying degrees) textures, meshes, animations, and skeleton
data found in ALRs. Polaris is designed to replace normal hex editors for ALR
modding, without giving up the flexibility. There are mini hex editors in
nearly every window, so you can quickly jump to the data you want (and see
only that data).

Support status:

| Feature           | Viewing                    | Export                             | Import                                   |
| ----------------- | -------------------------- | ---------------------------------- | ---------------------------------------- |
| Textures (square) | ✅                         | ✅(`.dds`)                         | ✅ (`.dds`, at same resolution & format) |
| Textures (atlas)  | ✅                         | ✅(`.dds`)                         | ❌                                       |
| Character meshes  | ✅                         | ✅(`.obj`)                         | ❌                                       |
| Stage meshes      | ✅                         | ✅ (`.obj`)                        | ❌                                       |
| Animations        | ✅ (graph)                 | ❌ (WIP `.anim` export)            | ❌                                       |
| Skeleton          | ❌                         | ✅ (`.dae`)                        | ❌                                       |

Here's what the UI for viewing/editing map files and textures looks like:    
<img src="https://github.com/user-attachments/assets/a1fa4f13-0791-4ced-89ed-757387500670" style="width:90%;"/>     
And the UI for texture atlases:    
<img src="https://github.com/user-attachments/assets/5d75910c-8339-4d1f-bf80-931227157220" style="width:70%;"/>


### Command-line Options
Polaris takes a filename as its first argument (which also means you can drag-and-drop
files onto the executable). You can also specify another flag to run a specific
operation in *headless* mode (no GUI):        
`polaris [alr filename] [flag]`

- `--dump-textures`
  - Dump all textures to a `textures` folder in DDS format.
- `--validate`
  - Run assertions and consistency checks across the whole file. Doesn't guarantee
    the file will work in the game, just that nothing looks wrong based on our
    current knowledge of ALRs.

## ALR [`src/alr`]
This program can export/replace textures, and split ALR files apart in a few different ways. It's been mostly obseleted by
Polaris, but is still more reliable for unusual files (like bosses) and uncommon
texture formats (like cubemaps). Here's some typical output:        
<img src="https://github.com/user-attachments/assets/ad8a35d6-a630-40bc-854e-3d56442ad21e" style="width:80%;"/>
<img src="https://github.com/user-attachments/assets/308ee1cc-4b43-44bf-a22d-b23312f284e3" style="width:70%;"/>

### Command-line Options
`alr [input file] [flag] [output file (if applicable)]`
- `--dump`
  - Dumps all textures from the file, guessing the best format and size using metadata from
    2 parts of the file. Has a bad habit of dumping one file with correct format but wrong size,
    and one with the wrong format and correct size. Use Polaris instead, except for cubemaps.
- `--info`
  - Does a "dry run", printing out info but not dumping any files.
- `--replace`
  - Searches for textures that you dumped earlier, and replaces textures in the ALR with those files.
    Allows you to dump textures, edit them, then replace them in the ALR.

## Deck Reader [`src/deck_reader`]
This is a very simple TUI editor for deck/arsenal files. In the style of a simple batch
script menu, you select a menu option by number and type in a new value. The menu updates
in-place instead of scrolling.

<img src="https://github.com/user-attachments/assets/8992a8e4-cddf-495b-99c2-db9998c41bda" style="width:70%;"/>

## SSB Tool [`src/ssb`]
This is even more barebones than the deck editor, because I don't work with SSBs often.
Our resident SSB expert is Vu, who has some Python tools [here](https://github.com/VSaige3/pd-ssb-decomp).
For more information on the file format, check out the [wiki page](https://phantomdust.miraheze.org/wiki/File_Formats/SSB).
The tool in this repo will only display an SSB's function export table:

<img src="https://github.com/user-attachments/assets/57e7bc37-d45b-413b-92fb-4d7d091a413b" style="width:70%;"/>
