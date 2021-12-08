#!/usr/bin/env python3

'''
pifconverter.py

Converts a PIF2 file to PNG.
THE FIRST BYTE OF THE FILE MUST BE THE FIRST BYTE OF THE PIF2 HEADER !
Use mass_pif_convert.c for batch convertions instead.

Released as Public Domain.
'''

from argparse import ArgumentParser
from collections import namedtuple
from pprint import pprint
from struct import Struct
from PIL import Image


_pif_header = Struct('<4s4x6I')
_palette_data = Struct('<3Bx')
_palette_color = namedtuple('Color', ['r', 'g', 'b'])
_bitmap_data = Struct('<B')


def main():
    parser = ArgumentParser(description='A simple converter from PIF to PNG.')
    parser.add_argument('pif_file',help='PIF file to convert')
    #parser.add_argument('-c','--color_format',help='Color format to parse',type=str) #Not done yet
    parser.add_argument('-s','--show_result',help='Displays the image before saving it.',action='store_true')
    parser.add_argument('-p','--save_palette',help='Save the palette in addition to the image.',action='store_true')
    args = parser.parse_args()

    with open(args.pif_file, 'rb') as f:
        data = f.read()

    magic, bitmap_width, bitmap_height, unk1, unk2, unk3, texture_type = _pif_header.unpack_from(data, 0)

    image_size = bitmap_width * bitmap_height

    offset = _pif_header.size
    entries_in_palette = 2**8 #Palette is always (?) 256 colors
    bitmap_palette = [_palette_color(*_palette_data.unpack_from(data, offset + (_palette_data.size * i))) for i in range(entries_in_palette)]
    #pprint(bitmap_palette)

    print("Magic: {magic}\nWidth: {bitmap_width} | Height: {bitmap_height}\nTexture type: {texture_type}\nSize of the bitmap : {image_size} byte(s)".format(**locals()))

    if args.save_palette:
        palette_image = Image.new('RGB',(1,entries_in_palette),None) #Stores the palette in a vertical line
        for i in range(entries_in_palette):
            color = bitmap_palette[i]
            palette_image.putpixel((0,i), (color.r, color.g, color.b))

        if args.show_result:
            palette_image.show()
        palette_image.save(args.pif_file + "-palette.png","PNG")
        print("Saved the palette to " + args.pif_file + "-palette.png !", sep="")
            
    

    offset = _pif_header.size + _palette_data.size * entries_in_palette
    pif_image = Image.new('RGB', (bitmap_width,bitmap_height),None)
    
    for i in range(bitmap_width*bitmap_height):
        x = i % bitmap_height #Yes, this is dirty. Sorry.
        y = i // bitmap_height
        palette_index = _bitmap_data.unpack_from(data, offset + i)[0] #Unpack returns a tuple but we want an int
        if (palette_index & 0x10) == 0x10 and (palette_index & 0x8) == 0:
            color = bitmap_palette[palette_index - 8]
        elif (palette_index & 0x10) == 0 and (palette_index & 0x8) == 0x8:
            color = bitmap_palette[palette_index + 8]
        else:
            color = bitmap_palette[palette_index]
        pif_image.putpixel((x, y), (color.r, color.g, color.b))


    if args.show_result:
        pif_image.show()
    pif_image.save(args.pif_file + ".png","PNG")
    print("Image exported with success to '" + args.pif_file + ".png'!", sep = "")


if __name__ == "__main__":
    main()
