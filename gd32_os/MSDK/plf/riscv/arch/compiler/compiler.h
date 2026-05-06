/*!
    \file    compiler.h
    \brief   Definitions of compiler specific directives.

    \version 2023-07-20, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2023, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#ifndef _COMPILER_H_
#define _COMPILER_H_

// define the force inlining attribute for this compiler
#ifndef __INLINE
#define __INLINE static __attribute__((__always_inline__)) inline
#endif

#define __STATIC static

// define size of an empty array (used to declare structure with an array size not defined)
#define __ARRAY_EMPTY

// define the BLE IRQ handler attribute for this compiler
#define __BLEIRQ

// define the IRQ handler attribute for this compiler
#define __IRQ __attribute__((interrupt))

// function has no side effect and return depends only on arguments
#define __PURE __attribute__((const))

// Align instantiated lvalue or struct member on 4 bytes
#define __ALIGN4 __attribute__((aligned(4)))

#define __ALIGN64 __attribute__((aligned(64)))

// Pack a structure field
#define __PACKED16 __attribute__ ((__packed__))
// Pack a structure field
#ifndef __PACKED
#define __PACKED __attribute__ ((__packed__))
#endif

// __MODULE__ comes from the RVDS compiler that supports it
#define __MODULE__ __BASE_FILE__

// define a variable as maybe unused, to avoid compiler warnings on it
#define __MAYBE_UNUSED __attribute__((unused))

// Mapping of these different elements is already handled in the map.txt file, so no need
// to define anything here
// SHARED RAM
#define __SHAREDRAM

// LA RAM
#define __LARAMMAC __attribute__ ((section("LARAM")))
// MIB memory
#define __MIB __attribute__ ((section("MACHWMIB")))

#endif // _COMPILER_H_
