#!/usr/bin/env python3

import json
from argparse import ArgumentParser
import sys
from typing import List, Optional

class Mapping(object):
    scancode:int
    ascii:int
    shifted_ascii:int

    @staticmethod
    def decode_extended(c:str) -> Optional[int]:
        """
        Map known control characters to ASCII equivalents
        """
        print("decode_extended: {}".format(c))
        if c=="ESC": 
            return 0x1b
        elif c=="BACKSPACE":
            return 0x08
        elif c=="TAB":
            return 0x09
        else:
            return None
        
    def __init__(self, scancode:str, ascii:str, shifted_ascii:str) -> None:
        self.scancode = int(scancode)
        if len(ascii)==1:
            self.ascii = ord(ascii)
        else:
            self.ascii = self.decode_extended(ascii)
            print("got: {}".format(self.ascii))
            if self.ascii is None:
                self.ascii = 0

        if len(shifted_ascii)==1:
            self.shifted_ascii = ord(shifted_ascii)
        else:
            self.shifted_ascii = self.decode_extended(shifted_ascii)
            if self.shifted_ascii is None:
                self.shifted_ascii = 0

    def to_bytes(self) -> bytes:
        # norm_bytes = bytes([0])
        # if self.ascii:
        #     norm_bytes = self.ascii.to_bytes(1, 'little')
        # shifted_bytes = bytes([0])
        # if self.shifted_ascii:
        #     shifted_bytes = self.shifted_ascii.to_bytes(1, 'little')
        # return bytes([norm_bytes, shifted_bytes])
        return bytes([self.ascii, self.shifted_ascii])
    
def load_json(path:str) -> tuple[dict, int]:
    with open(path, "r") as f:
        content = json.loads(f.read())

        mappings = {}
        highest = 0
        for sc_string, codes in content.items():
            m = Mapping(sc_string, codes["normal"], codes["shifted"])
            mappings[m.scancode] = m
            if m.scancode>highest:
                highest = m.scancode

        return mappings, highest
    
parser = ArgumentParser()
parser.add_argument("-i", "--input", help="JSON file to read in")
parser.add_argument("-o", "--output", help="binary file to output", default="keys.bin")
args = parser.parse_args()

if args.input=="":
    print("You must specify an input file. Try running with --help")
    sys.exit(1)

mappings, highest_mapping = load_json(args.input)

print("Highest scancode is {}".format(highest_mapping))
print("Total number of scancodes is {}".format(len(mappings)))

# FIXME - not right yet.  We need to insert 'spaces' where we have no keys.
# The binary file can be read like this:
# bytes 0,1 - 16-bit length of the kb map
# therafter, even bytes are for the 'normal' key and odd for the 'shifted' key.
# So, the conversion for scancode 0x2 is normal 0x02 and shifted 0x03, 0x3 is normal 0x04 shifted 0x05, etc.
with open(args.output, "wb") as f:
    f.write(bytes(len(mappings).to_bytes(2, byteorder='little')))

    for i in range(1, highest_mapping):
        if i in mappings:
            f.write(mappings[i].to_bytes())
        else:
            f.write(bytes([0,0]))