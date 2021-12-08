#!/usr/bin/env python3

'''
vram2gxt.py

Converts a VRAM+PS3 file pair into a GXT (texture container for PS Vita)
GXT files can be converted to PNG with i.e. Scarlet : https://github.com/xdanieldzd/Scarlet

Released as Public Domain.
'''

from argparse import ArgumentParser
from struct import Struct

_ps3_file = Struct('<4xI4x2I4x2HI')
_gxt_header = Struct('<4s4B6I')
_gxt_texture_header = Struct('<6I4H')

SINGLETEX_DESCRIPTOR_SIZE = 0x20

def main():
    parser = ArgumentParser(description='Adds a GXT header to .VRAM files, allowing you to late convert them with i.e. Scarlet.')
    parser.add_argument('filename',help='Name of the PS3/VRAM pair to convert. Extension is automatically stripped.')
    args = parser.parse_args()

    if (args.filename.endswith(".ps3")):
        ps3Filename = args.filename
        vramFilename = args.filename[:-3] + "vram" #Replace ps3 with vram

    elif (args.filename.endswith(".vram")):
        ps3Filename = args.filename[:-4] + "ps3" #Replace vram with ps3
        vramFilename = args.filename

    else:
        print("Error : '%s' doesn't end with expected extension '.ps3' or '.vram'" % args.filename)
        return

    outFilename = vramFilename + ".gxt"

    try:
        ps3fh = open(ps3Filename, "rb")
        vramfh = open(vramFilename, "rb")
        outfh = open(outFilename, "wb")
    except:
        print("Error opening one or more files.")
        return

    ps3fdata = ps3fh.read(32)
    ps3fh.close()

    texFmt, mipCount, texType, height, width, texSize = _ps3_file.unpack(ps3fdata)
    print("Texture format 0x%08X | Texture type 0x%08X | %d Mipmaps | Dimensions: %dx%d | Size : %d" % (texFmt, texType, mipCount, width, height, texSize))
    
    #Write out expected GXT header - format described here: https://playstationdev.wiki/psvitadevwiki/index.php/GXT

    outfh.write(_gxt_header.pack(
        bytes("GXT\0", "ascii"), #GXT\0 magic
        0x03, 0x00, 0x00, 0x10, #Version 3.01 (0x03000010)
        1, #Number of embedded textures (only one)
        0x40, #Offset to texture data (= size of both headers)
        texSize, #Total size of textures
        0, #Number of 4-bit palettes
        0, #Number of 8-bit palettes
        0  #Padding
        ))
    outfh.write(_gxt_texture_header.pack(
        0x40, #Offset to this texture's data (= size of both headers)
        texSize, #Size of this texture
        0xFFFFFFFF, #Index of palette (-1 = no palette)
        0, #Flags/unused
        texType, #Texture type
        texFmt, #Texture base format
        width, #Texture width
        height, #Texture height
        mipCount, #Number of mipmaps
        0 #Padding
        ))
    outfh.write(vramfh.read())
    vramfh.close()
    outfh.close()

    print("Finished !")

if __name__ == "__main__":
    main()