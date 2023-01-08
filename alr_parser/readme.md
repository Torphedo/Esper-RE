# ALR Parser

This tool allows you to get information about the layout/contents of a `.alr` file, and dump various forms of data from it.
Current supported formats to dump are:
  - 3D Animation data
  - Texture data
Note: texture data is currently dumped using the filenames from the ALR, but are always formatted in TGAs. All `.dds` files
outputted need to be renamed to `.tga` to be opened.

This tool is command-line / drag-and-drop only.    

## Usage

`alr_parser <filepath> --[option]`    

  `filepath`:    
      Must be a path to a `.alr` file.
  
  `option`:    
      Can be either `split`, `info`, or `dump`.
  
   `split`:     
      Writes out each large chunk of data pointed to in the header to separate files with a `.bin` extension.
    
   `info`:     
      Parses the entire file, giving information on the location and type of each data block, texture metadata & filenames,    
      and the address stored in the header where filler data ends
   
   `dump`:          
      Parses the entire file. Only prints texture metadata to the console, but will also write animation data to a text file,      
      and dump TGA files with texture data using the names in the ALR file. The texture data will usually be unusable, but     
      the first few images in the list printed to console will often be complete or semi-complete. A good example of usable    
      texture dumps comes from `FDLogo.alr`, where the outputted `logo_02.dds` (after renaming to `logo_02.tga` will contain    
      a correct dump of the game's logo from the title screen.
      
If you simply drag-and-drop an ALR file onto the tool, it will default to the `split` option.
