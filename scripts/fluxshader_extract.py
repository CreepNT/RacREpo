from contextlib import redirect_stdout
from enum import Enum
import argparse
import os
import struct

class UniformMetadataTableHdr:
    class UniformMetadata:
        STRUCTURE = struct.Struct("<4I")
        SIZE = STRUCTURE.size

        def __init__(self, b: bytes, offset: int):
            assert(self.SIZE == 0x10)
            self._startOffset = offset

            (
                _offsetToName,
                self.unk4,
                self.unk8,
                self.unkC
            ) = self.STRUCTURE.unpack_from(b, offset)

            nameStartOffset = self._startOffset + _offsetToName
            NULOffset = nameStartOffset + b[nameStartOffset:].find(b'\0')
            self.name = b[nameStartOffset:NULOffset].decode("utf-8")

        def __str__(self):
            res  = f"         Uniform Metadata @ 0x{self._startOffset:X}:\n"
            res += f"          Name: {self.name}\n"
            res += f"          Unk4: 0x{self.unk4:X}\n"
            res += f"          Unk8: 0x{self.unk8:X}\n"
            res += f"          UnkC: 0x{self.unkC:X}\n"
            return res            

    STRUCTURE = struct.Struct("<5I")
    SIZE = STRUCTURE.size

    def __init__(self, b: bytes, offset: int):
        assert(self.SIZE == 0x14)
        self._startOffset = offset

        (
            self.unk0,
            self.unk4,
            self.offsetToUniforms,
            self.numUniforms,
            self.unk10
        ) = self.STRUCTURE.unpack_from(b, offset)

        self._fields = []
        for i in range(self.numUniforms):
            #+0x8 = offsetof(self.offsetToUniforms)
            uniformOffset = self._startOffset + 0x8 + self.offsetToUniforms + i * self.UniformMetadata.SIZE
            self._fields.append(self.UniformMetadata(b, uniformOffset))

    def __str__(self) -> str:
        res  = f"       Uniform Metadata Table Header @ 0x{self._startOffset:X}:\n"
        res += f"        Unk0:  0x{self.unk0:X}\n"
        res += f"        Unk4:  0x{self.unk4:X}\n"
        res += f"        Unk10: 0x{self.unk10:X}\n"
        res += f"        Offset to uniforms: 0x{self.offsetToUniforms:X}\n"
        res += f"        Number of uniforms: {self.numUniforms}\n"
        for uniform in self._fields:
            res += str(uniform)
        return res

class GXPHdr:
    STRUCTURE = struct.Struct("2I12xI8x")
    SIZE = STRUCTURE.size

    def __init__(self, b: bytes, offset: int, size: int):
        assert(self.SIZE == 0x20)
        self._startOffset = offset
        self._size = size

        (
            _offsetToGXP,
            self.unk4,
            #unk8  is dummy in file - when loaded, holds SceGxmShaderPatcherId
            #unkC  is dummy in file - when loaded, set to -(&pGXPHdr->unkC)
            #unk10 is dummy in file - when loaded, set to 0
            self.unk14,
            #unk18 is dummy in file - when loaded, set to -(&pGXPHdr->unk18)
            #unk1C is dummy in file - when loaded, set to 0
        ) = self.STRUCTURE.unpack_from(b, offset)

        if (_offsetToGXP != 0x20):
            raise ValueError(f"!!!!!Unexpected GXP header 'offsetToGXP' 0x{_offsetToGXP}!!!!!\nPlease report this to CreepNT!")

        self._GXPBytes = b[offset + self.SIZE:offset + size] #Don't add self.SIZE to max bound because we want (size - self.SIZE) bytes
        assert(len(self._GXPBytes) == (size - self.SIZE))
        if (self._GXPBytes[0:3] != b'GXP'):
            raise ValueError("Malformed GXM program!")

    def __str__(self) -> str:
        res  = f"       GXP Header @ 0x{self._startOffset:X} (blob size 0x{self._size:X}):\n"
        res += f"        Unk4:  0x{self.unk4:X}\n"
        res += f"        Unk14: 0x{self.unk14:X}\n"
        #res +=  "        GXM Program bytes: " + str(self._GXPBytes) + "\n"
        return res
        
class ShaderHdr:
    STRUCTURE = struct.Struct("<4s7I")
    SIZE = STRUCTURE.size

    def __init__(self, b: bytes, offset: int):
        assert(self.SIZE == 0x20)
        self._startOffset = offset

        (
            self.magic,
            self.unk4,
            self.unk8,
            self.offsetToGXPHdr, #Offset *from this field* to the GXP header
            self.GXPBlobSize,    #Size of GXP header + shader GXP binary, starting at offsetToGXPHdr
            self.offsetToUniformMeta,
            self.unk18,
            self.unk1C
        ) = self.STRUCTURE.unpack_from(b, offset)

        if (self.magic != b'FLUX'):
            raise ValueError("Invalid ShaderHdr magic.")

        self._GXPHdr = GXPHdr(b, self._startOffset + 0xC + self.offsetToGXPHdr, self.GXPBlobSize) #0xC = offsetof(offsetToGXPHdr)
        self._UniformMeta = UniformMetadataTableHdr(b, self._startOffset + 0x14 + self.offsetToUniformMeta) #0x14 = offsetof(self.offsetToUniformMeta)

    def __str__(self) -> str:
        res  = f"    Shader Header @ 0x{self._startOffset:X}:\n"
        res += f"     Unk4:  0x{self.unk4:X}\n"
        res += f"     Unk8:  0x{self.unk8:X}\n"
        res += f"     Unk18: 0x{self.unk18:X}\n"
        res += f"     Unk1C: 0x{self.unk1C:X}\n"
        res +=  "     GXP Header:\n"
        res += f"      Offset: 0x{self.offsetToGXPHdr:X}\n"
        res += f"      Size:   0x{self.GXPBlobSize:X}\n"
        res += f"      Contents:\n"
        res += str(self._GXPHdr)
        res += f"     Uniform Metadata offset: 0x{self.offsetToUniformMeta:X}\n"
        res += str(self._UniformMeta)
        return res

class SecondaryHeader:
    STRUCTURE = struct.Struct("<3I")
    SIZE = STRUCTURE.size

    def __init__(self, b: bytes, offset: int):
        assert(self.SIZE == 0xC)
        self._startOffset = offset

        (
            self.unk0,
            self.vertexShaderOffset, #Offset from file start
            self.pixelShaderOffset   #Offset from file start
        ) = self.STRUCTURE.unpack_from(b, self._startOffset)

        self._vertexShaderHdr = ShaderHdr(b, self.vertexShaderOffset)
        self._pixelShaderHdr = ShaderHdr(b, self.pixelShaderOffset)


    def __str__(self) -> str:
        res  = f"  Secondary Header @ 0x{self._startOffset:X}:\n"
        res += f"   Unk0: {self.unk0}\n"
        res += f"   Vertex Shader Offset: 0x{self.vertexShaderOffset:X}\n"
        res += str(self._vertexShaderHdr)
        res += f"   Pixel Shader Offset: 0x{self.pixelShaderOffset:X}\n"
        res += str(self._pixelShaderHdr)
        return res

class FileHeader:
    STRUCTURE = struct.Struct("<4s2I64s")
    SIZE = STRUCTURE.size

    def __init__(self, b: bytes):
        assert(self.SIZE == 0x4C)

        (
            self.magic,
            self.unk4,
            self.numSecondaryHdrs,
            _RawName
        ) = self.STRUCTURE.unpack_from(b, 0)

        if (self.magic != b'FLUX'):
            raise ValueError("Invalid file header magic")
        
        self.name = _RawName.decode("utf-8")

        self._secondaryHeaders = []
        for i in range(self.numSecondaryHdrs):
            self._secondaryHeaders.append(SecondaryHeader(b, self.SIZE + i * SecondaryHeader.SIZE))

    def __str__(self) -> str:
        res  =  "File Header:\n"
        res += f" Name: {self.name}\n"
        res += f" Unk4: 0x{self.unk4}\n"
        res += f" {self.numSecondaryHdrs} secondary header(s):\n"
        for sHdr in self._secondaryHeaders:
            res += str(sHdr)
        return res

class ShaderType(Enum):
    VERTEX_SHADER = 0
    PIXEL_SHADER = 1

def write_gxp_to_folder(folder: str, sHdrIndex: int, gxpHdr: GXPHdr, type: ShaderType):
    if (type == ShaderType.VERTEX_SHADER):
        shaderType = "vp"
    else:
        shaderType = "fp"

    filePath = folder + f"shader_{sHdrIndex}_{shaderType}_GXP0x{gxpHdr._startOffset + gxpHdr.SIZE:X}.gxp"
    with open(filePath, "wb") as outfd:
        outfd.write(gxpHdr._GXPBytes)

def main():
    parser = argparse.ArgumentParser(description="Parses and extracts data from a Flux shader file (.flux).")
    parser.add_argument("infile", type=argparse.FileType("rb"), help="Input Flux shader file")
    parser.add_argument("-o", "--output", "--output-file", dest="outfile", type=argparse.FileType("w"), help="Output file (.txt) - default is stdout.")
    parser.add_argument("--gxp-output", dest="gxpOutFolder", type=str, help="Destination folder for GXP files")

    args = parser.parse_args()
    fileHdr = FileHeader(args.infile.read())
    if (args.outfile != None):
        with redirect_stdout(args.outfile):
            print(fileHdr)
    else:
        print(fileHdr)

    if (args.gxpOutFolder != None):
        folder: str = args.gxpOutFolder
        if not folder.endswith("/") and not folder.endswith("\\"):
            folder += os.path.sep

        if not os.path.exists(folder):
            os.mkdir(folder)

        for i in range(len(fileHdr._secondaryHeaders)):
            sHdr: SecondaryHeader = fileHdr._secondaryHeaders[i]
            write_gxp_to_folder(folder, i, sHdr._vertexShaderHdr._GXPHdr, ShaderType.VERTEX_SHADER)
            write_gxp_to_folder(folder, i, sHdr._pixelShaderHdr._GXPHdr, ShaderType.PIXEL_SHADER)

if __name__ == "__main__":
    main()