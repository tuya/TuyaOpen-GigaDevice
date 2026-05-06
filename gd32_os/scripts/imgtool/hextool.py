#! /usr/bin/env python3
#
#     Copyright (c) 2024, GigaDevice Semiconductor Inc.
# 
#     Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
#     1. Redistributions of source code must retain the above copyright notice, this
#        list of conditions and the following disclaimer.
#     2. Redistributions in binary form must reproduce the above copyright notice,
#        this list of conditions and the following disclaimer in the documentation
#        and/or other materials provided with the distribution.
#     3. Neither the name of the copyright holder nor the names of its contributors
#        may be used to endorse or promote products derived from this software without
#        specific prior written permission.
#
#     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
# OF SUCH DAMAGE.


import os
import re
import subprocess
import argparse
import macro_parser 

flash_base_re = re.compile(r"^.*\s*RE_FLASH_BASE\s*(.*)")
offset_re = re.compile(r"^.*\s*RE_([0-9A-Z_]+)_OFFSET\s*(.*)")

def convert_to_hex(args):
    flash_base = macro_parser.evaluate_macro(args.config, flash_base_re, 0, 1)
    offsets = macro_parser.evaluate_macro(args.config, offset_re, 1, 2)
    act_offset = offsets[args.type] + flash_base
    cmd = '"' + args.exe + '" "' + args.infile + '" -Binary -offset ' + str(hex(act_offset)) + ' -o "' + args.outfile + '" -Intel'
    #print(cmd)
    subprocess.run(cmd, stdin=None, input=None, stdout=None, stderr=None, shell=True, timeout=None, check=False)    

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--config', metavar='filename', required=True,
                          help='Location of the file that contains macros')
    parser.add_argument('-t', '--type', metavar='type', required=True,
                          help='SYS_SET / MBL / SYS_STATUS / IMG_0 / IMG_1')
    parser.add_argument('-e', '--exe', metavar='filename', required=True,
                          help='Location of the file that contains macros')    
    parser.add_argument("infile")
    parser.add_argument("outfile")
    
    args = parser.parse_args()
    convert_to_hex(args)

if __name__ == '__main__':
    main()