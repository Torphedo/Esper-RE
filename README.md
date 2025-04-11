# Esper-RE
A repository containing tools, templates, and information for editing and reverse-engineering Phantom Dust files

## Polaris
This is the main program for this repo, a graphical editor for ALR files. It
understands (to varying degrees) textures, meshes, animations, and skeleton
data found in ALRs. It has built-in hex editors all over the UI, but also an
intuitive interface sitting on top.

Support status:

| Feature           | Viewing                           | Export          | Import                                  |
| ----------------- | --------------------------------- | --------------- | --------------------------------------- |
| Textures (square) | Supported                         | Supported       | Supported (at same resolution & format) |
| Textures (atlas)  | Supported                         | Unsupported     | Unsupported                             |
| Character meshes  | Supported                         | Supported       | Unsupported                             |
| Stage meshes      | Supported                         | Supported (WIP) | Unsupported                             |
| Animations        | Supported (translate/rotation)    | Unsupported     | Unsupported                             |
| Skeleton          | Early WIP                         | Supported (WIP) | Unsupported                             |

You can export all textures from a file on the command-line:
`polaris [alr filename] --dump-textures`

## ALR
This is a not very well named command-line program that can export/replace
textures and split files apart in different ways. It's mostly obselete, but is
more accurate for unusual ALRs (like bosses) and uncommon texture formats (like
cubemaps). Check the `alr` folder for more details.