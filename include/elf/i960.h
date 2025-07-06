/* Intel 960 ELF support for BFD.
   Copyright (C) 1999-2022 Free Software Foundation, Inc.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.  */

#ifndef _ELF_I960_H
#define _ELF_I960_H

#include "elf/reloc-macros.h"


START_RELOC_NUMBERS (elf_i960_reloc_type)
     RELOC_NUMBER (R_960_NONE,	    0)
     RELOC_NUMBER (R_960_12,	    1)
     RELOC_NUMBER (R_960_32,	    2)
     RELOC_NUMBER (R_960_IP24,	    3)
     RELOC_NUMBER (R_960_SUB,	    4)
     RELOC_NUMBER (R_960_OPTCALL,   5)
     RELOC_NUMBER (R_960_OPTCALLX,  6)
     RELOC_NUMBER (R_960_OPTCALLXA, 7)
     /* Custom Reloc Numbers for rv32 static translator start here */
     RELOC_NUMBER (R_960_RV32_HI20, 8)
     RELOC_NUMBER (R_960_RV32_LO12, 9)
END_RELOC_NUMBERS (R_960_max)

/* Object attributes (taken from riscv for emulation purposes) .  */
enum
{
  /* 0-3 are generic.  */
  Tag_I960_stack_align = 4,
  Tag_I960_arch = 5,
  Tag_I960_unaligned_access = 6,
  Tag_I960_priv_spec = 8,
  Tag_I960_priv_spec_minor = 10,
  Tag_I960_priv_spec_revision = 12
};
#endif /* _ELF_I960_H */
