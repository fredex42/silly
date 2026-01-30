#!/usr/bin/env python3
from argparse import ArgumentParser

parser = ArgumentParser()
parser.add_argument("-i","--input", help="Binary file to read in", default="keys.bin")
args = parser.parse_args()

with open(args.input, "rb") as input:
    rawdata = input.read()

print("static var = {", end='')

for b in rawdata:
    print("0x{:02X}".format(b), end=",")

print("}")