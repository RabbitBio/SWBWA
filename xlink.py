import traceback
import struct
import sys
import re
import collections
import enum
import ctypes
import argparse

ELF64_HDR_FMT = "10sHHIQQQIHHHHHH"
ELF64_Hdr = collections.namedtuple('ELF64_Hdr', ['e_ident', 'e_type', 'e_machine', 'e_version',
                                                 'e_entry', 'e_phoff', 'e_shoff', 'e_flags',
                                                 'e_ehsize', 'e_phentsize', 'e_phnum', 'e_shentsize',
                                                  'e_shnum', 'e_shstrndx'])
ELF64_SHDR_FMT = "IIQQQQIIQQ"
ELF64_Shdr = collections.namedtuple('ELF64_Shdr', ['sh_name', 'sh_type', 'sh_flags', 'sh_addr',
                                                   'sh_offset', 'sh_size', 'sh_link', 'sh_info',
                                                   'sh_addralign', 'sh_entsize'])

ELF64_ST_BIND = lambda x: (x & 0xff) >> 4
ELF64_ST_TYPE = lambda x: x & 0xf
ELF64_ST_INFO = lambda bind, type: bind << 4 | type & 0xf

ELF64_SYM_FMT = "IBBHQQ"
ELF64_Sym = collections.namedtuple('ELF64_Sym', ['st_name', 'st_info', 'st_other', 'st_shndx', 'st_value', 'st_size'])

STB_LOCAL      = 0               # Local symbol */
STB_GLOBAL     = 1               # Global symbol */
STB_WEAK       = 2               # Weak symbol */
STB_NUM        = 3               # Number of defined types.  */
STB_LOOS       = 10              # Start of OS-specific */
STB_GNU_UNIQUE = 10              # Unique symbol.  */
STB_HIOS       = 12              # End of OS-specific */
STB_LOPROC     = 13              # Start of processor-specific */
STB_HIPROC     = 15              # End of processor-specific */

STT_NOTYPE     = 0               # Symbol type is unspecified */
STT_OBJECT     = 1               # Symbol is a data object */
STT_FUNC       = 2               # Symbol is a code object */
STT_SECTION    = 3               # Symbol associated with a section */
STT_FILE       = 4               # Symbol's name is file name */
STT_COMMON     = 5               # Symbol is a common data object */
STT_TLS        = 6               # Symbol is thread-local data object*/
STT_NUM        = 7               # Number of defined types.  */
STT_LOOS       = 10              # Start of OS-specific */
STT_GNU_IFUNC  = 10              # Symbol is indirect code object */
STT_HIOS       = 12              # End of OS-specific */
STT_LOPROC     = 13              # Start of processor-specific */
STT_HIPROC     = 15     # End of processor-specific */


class ELF64:
    def __init__(self, bin):
        bin = bytearray(bin)
        ehdr = ELF64_Hdr(*struct.unpack(ELF64_HDR_FMT, bin[0:64]))
        self.bin = bin
        self.ehsize    = ehdr.e_ehsize
        self.phoff     = ehdr.e_phoff
        self.phnum     = ehdr.e_phnum
        self.phentsize = ehdr.e_phentsize
        self.shoff     = ehdr.e_shoff
        self.shnum     = ehdr.e_shnum
        self.shentsize = ehdr.e_shentsize

        shdr_end = self.shoff + self.shnum * self.shentsize
        shdr = list(map(lambda x: ELF64_Shdr(*x), struct.iter_unpack(ELF64_SHDR_FMT, bin[self.shoff : shdr_end])))
        shstrtabhdr = shdr[ehdr.e_shstrndx]
        #shstrdict = ELF64.split_strtab(
        shstrtab = ELF64.get_section_bin(bin, shstrtabhdr)
        self.shstrtab = shstrtab
        #print(shstrtab[341:351], len(shstrtab))
        #print("TTT:", shstrtab[341:20])
        self.shdrs = {}
        self.ishdrs = []
        for hdr in shdr:
            # print(hdr, hdr.sh_name, ELF64.get_str_from_tab(shstrtab, hdr.sh_name))
            self.shdrs[ELF64.get_str_from_tab(shstrtab, hdr.sh_name)] = hdr
            self.ishdrs.append(hdr)
        symtabhdr = self.shdrs['.symtab']
        symtabbin = ELF64.get_section_bin(bin, symtabhdr)
        syms = list(map(lambda x: ELF64_Sym(*x), struct.iter_unpack(ELF64_SYM_FMT, symtabbin)))
        strtabhdr = self.shdrs['.strtab']
        #strdict = ELF64.split_strtab(
        
        strtab = ELF64.get_section_bin(bin, strtabhdr)
        self.strtab = strtab
        #self.sym_name = {}
        self.syms = list(map(lambda sym: sym._replace(st_name = ELF64.get_str_from_tab(strtab, sym.st_name)), syms))
        self.global_syms = {}
        for sym in self.syms:
            if ELF64_ST_BIND(sym.st_info) in [STB_GLOBAL, STB_WEAK]:
                self.global_syms[sym.st_name] = sym
    @staticmethod
    def get_str_from_tab(tab, index):
        return tab[index:].split(b"\0", 1)[0].decode()

    @staticmethod
    def get_section_bin(elf_bin, sec_hdr):
        return elf_bin[sec_hdr.sh_offset : sec_hdr.sh_offset + sec_hdr.sh_size]

class SW9Rel(enum.Enum):
    NONE       = 0  # /* No reloc */
    REFLONG    = 1  # /* Direct 32 bit */
    REFQUAD    = 2  # /* Direct 64 bit */
    GPREL32    = 3  # /* GP relative 32 bit */
    LITERAL    = 4  # /* GP relative 16 bit w/optimization */
    LITUSE     = 5  # /* Optimization hint for LITERAL */
    GPDISP     = 6  # /* Add displacement to GP */
    BRADDR     = 7  # /* PC+4 relative 23 bit shifted */
    HINT       = 8  # /* PC+4 relative 16 bit shifted */
    SREL16     = 9  # /* PC relative 16 bit */
    SREL32     = 10 # /* PC relative 32 bit */
    SREL64     = 11 # /* PC relative 64 bit */
    BR23ADDR   = 14 # /* PC relative 64 bit */
    GPRELHIGH  = 17 # /* GP relative 32 bit, high 16 bits */
    GPRELLOW   = 18 # /* GP relative 32 bit, low 16 bits */
    GPREL16    = 19 # /* GP relative 16 bit */
    COPY       = 24 # /* Copy symbol at runtime */
    GLOB_DAT   = 25 # /* Create GOT entry */
    JMP_SLOT   = 26 # /* Create PLT entry */
    RELATIVE   = 27 # /* Adjust by program base */
    TLS_GD_HI  = 28
    TLSGD      = 29
    TLS_LDM    = 30
    DTPMOD64   = 31
    GOTDTPREL  = 32
    DTPREL64   = 33
    DTPRELHI   = 34
    DTPRELLO   = 35
    DTPREL16   = 36
    GOTTPREL   = 37
    TPREL64    = 38
    TPRELHI    = 39
    TPRELLO    = 40
    TPREL16    = 41
    LDMLO      = 42
    LDMHI      = 43
    TLSHI      = 44
    TLSLO      = 45

LDL_MASK = 4026531847
LDL_OPCODE = 3221225476
LDI_MASK =  4026531840
LDI_OPCODE = 3758096384

def process_got(elf):
    text1_addr = elf.shdrs['.text1'].sh_addr
    text1_offset = elf.shdrs['.text1'].sh_offset
    text1_size = elf.shdrs['.text1'].sh_size

    got_addr = elf.shdrs['.got'].sh_addr
    got_offset = elf.shdrs['.got'].sh_offset
    got_size = elf.shdrs['.got'].sh_size
    
    #print('text1_addr = ', hex(text1_addr))
    #print('text1_offset = ', hex(text1_offset))
    #print('text1_size = ', hex(text1_size))

    #print('got_addr = ', hex(got_addr))
    #print('got_offset = ', hex(got_offset))
    #print('got_size = ', hex(got_size))

    liters = []
    if '.rela.text1' in elf.shdrs:
        rela_text1_offset = elf.shdrs['.rela.text1'].sh_offset
        rela_text1_size = elf.shdrs['.rela.text1'].sh_size
        # find all literal type relocations and their addresses in .text1 section
        for i in range(rela_text1_offset, rela_text1_offset + rela_text1_size, 24):
            addr, info, off = struct.unpack("<QQQ", elf.bin[i : i + 24])
            reltype = SW9Rel(info & 63)
            if reltype == SW9Rel.LITERAL:
                liters.append(addr)
                #print(hex(addr), ' ', elf.syms[target])

    #got_addrs = {}
    addrs = []
    # filter out ldl ldi instructions and save the corresponding address
    for liter in liters:
        faddr = liter - text1_addr + text1_offset
        inst = struct.unpack('<I', elf.bin[faddr : faddr + 4])[0]
        addr = 0
        if (inst & LDL_MASK) == LDL_OPCODE:
            addr = inst & 0xfff8
        elif (inst & LDI_MASK) == LDI_OPCODE:
            addr = inst & 0xffff

        if addr not in addrs:
            addrs.append(addr)
            print(hex(liter), struct.unpack('h', struct.pack('H', addr))[0])

        #if val not in addrs:
        #    addrs.append((content & 0xffff) - 4)
        #    print(hex(liter), struct.unpack('h', struct.pack('H', (content & 0xffff) - 4))[0])
        ##got_addrs[liter] = content & 0xffff

    data = struct.pack('<Q', len(addrs))
    for addr in addrs:
        data = data + struct.pack('<H', addr)
    #data = struct.pack('<Q', len(got_addrs))
    #for (k, v) in got_addrs.items():
    #    data = data + struct.pack('<QQ', *[k, v])

    open('data.bin', 'wb').write(data)
        #print('rela_text1_size = ', hex(rela_text1_size))
        #addr, info, off = struct.unpack("<QQQ", elf.bin[rela_text1_offset : rela_text1_offset + 24])
        #print('addr = ', hex(addr))
        #print('off = ', hex(off))
        #print('info = ', hex(info))

def parse_args():
    parser = argparse.ArgumentParser(description="Process ELF file")
    parser.add_argument('f', type=str, help="Path to the ELF file")
    return parser.parse_args()


if __name__ == '__main__':
    args = parse_args()
    f = args.f
    elf = ELF64(open(f, 'rb').read())
    process_got(elf)
    #print(elf.global_syms)

