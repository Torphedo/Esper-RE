# ALR Parser

This tool allows you to get information about the layout/contents of a `.alr` file, and dump various forms of data from it.
Current supported formats to dump are:
  - 3D Animation data
  - Texture data

This tool is command-line / drag-and-drop only.    

## Usage

`alr_parser [options]`    

You can specify filenames and flags in any order, but flags have to be passed starting with `--` (such as `--info`). The following options are available:
 
   `split`:     
      Writes out each large chunk of data pointed to in the header to separate files with a `.bin` extension. Also dumps raw binary resources
      (usually textures) to `resource_[index].bin` (such as `resource_0.bin`, `resource_5.bin`, etc).
    
   `info`:     
      Parses the entire file, giving information on the location and type of each data chunk, texture metadata & filenames,    
      and the location stored in the header where resources like textures are stored. This will not write anything to disk.
   
   `dump`:          
      Parses the entire file. Prints texture metadata to the console, write animation data to a text file,      
      and dump DDS/TGA textures using the names in the ALR file. Textures will usually be viewable.
      
   `silent`:
      Silences all console output.
   
   `tga`:
      In `dump` mode, output textures as TGA files. This will still use the name in the file to write to disk, so you'll have to rename them
      from `.dds` to `.tga` to open them.
   
   `dds`:
      In `dump` mode, output textures as DDS files. This is the default mode.
      
If you simply drag-and-drop an ALR file onto the tool, it will default to the `split` option.
