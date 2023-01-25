# ALR Parser

This tool allows you to get information about the layout/contents of a `.alr` file, and dump various forms of data from it.
Current supported formats to dump are:
  - 3D Animation data
  - Texture data

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
      and the location stored in the header where resources like textures are stored. This will not write anything to disk.
   
   `dump`:          
      Parses the entire file. Prints texture metadata to the console, but will also write animation data to a text file,      
      and dump DDS/TGA textures using the names in the ALR file. These are sometimes unusable, because we don't know exactly      
	  how mipmaps are supposed to be read. This has been extensively tested on `FDLogo.alr` and `Title.alr`, but many other     
	  files with textures will break.
      
If you simply drag-and-drop an ALR file onto the tool, it will default to the `split` option.
