/* Disassemble i80960 instructions.
   Copyright (C) 1990-2017 Free Software Foundation, Inc.
   Updated by Joshua Scoggins 2022

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to the
   Free Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "sysdep.h"
#include "disassemble.h"

static const char *const reg_names[] = {
/*  0 */	"pfp", "sp",  "rip", "r3",  "r4",  "r5",  "r6",  "r7",
/*  8 */	"r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
/* 16 */	"g0",  "g1",  "g2",  "g3",  "g4",  "g5",  "g6",  "g7",
/* 24 */	"g8",  "g9",  "g10", "g11", "g12", "g13", "g14", "fp",
/* 32 */	"pc",  "ac",  "ip",  "tc",  "fp0", "fp1", "fp2", "fp3"
};


//static FILE *stream;		/* Output goes here */
//static struct disassemble_info *info;
static void print_addr (bfd_vma, disassemble_info*);
static void ctrl (bfd_vma, uint32_t, uint32_t, disassemble_info*);
static void cobr (bfd_vma, uint32_t, uint32_t, disassemble_info*);
static void reg (uint32_t, disassemble_info*);
static int mem (bfd_vma, uint32_t, uint32_t, int, disassemble_info*);
static void ea (bfd_vma, int, const char *, const char *, int, unsigned int, disassemble_info*);
static void dstop (int, int, int, disassemble_info*);
static void regop (int, int, int, int, disassemble_info*);
static void invalid (int, disassemble_info*);
static int pinsn (bfd_vma, uint32_t, uint32_t, disassemble_info*);
static void put_abs (uint32_t, uint32_t, disassemble_info*);
static int get_opcode_high4bits(uint32_t);

int
get_opcode_high4bits(uint32_t word) {
    return (word >> 28) & 0x0F;
}
/* Print the i960 instruction at address 'memaddr' in debugged memory,
   on INFO->STREAM.  Returns length of the instruction, in bytes.  */

int
print_insn_i960 (bfd_vma memaddr, disassemble_info *info)
{
    uint32_t word2 = 0xdeadbeef;
    bfd_byte buffer[4];

    /* Read word1.  Only read word2 if the instruction
       needs it, to prevent reading past the end of a section.  */

    int status = (*info->read_memory_func) (memaddr, buffer, 4, info);
    if (status != 0)
    {
        (*info->memory_error_func) (status, memaddr, info);
        return -1;
    }

    uint32_t word1 = bfd_getl32 (buffer);

    /* Divide instruction set into classes based on high 4 bits of opcode.  */
    switch (get_opcode_high4bits(word1)) {
        default:
            break;
        case 0x8:
        case 0x9:
        case 0xa:
        case 0xb:
        case 0xc:
            /* Read word2 by overwriting the buffer.  */
            /* Attempt to read the next word, read_memory_func can fail given the disassembly info
             * so don't worry and just break. However, on success we need to overwrite word2 */
            status = (*info->read_memory_func) (memaddr + 4, buffer, 4, info);
            if (status == 0) {
                /* okay so we got more bytes, overwrite word2 */
                word2 = bfd_getl32 (buffer);
            }
            break;
    }

    return pinsn( memaddr, word1, word2, info);
}


/*****************************************************************************
 *	All code below this point should be identical with that of
 *	the disassembler in gdmp960.

 A noble sentiment, but at least in cosmetic ways (info->fprintf_func), it
 just ain't so. -kingdon, 31 Mar 93
 *****************************************************************************/

struct tabent {
  char *name;
  short numops;
};

struct sparse_tabent {
  int opcode;
  char *name;
  short numops;
};

static int
pinsn (bfd_vma memaddr, uint32_t word1, uint32_t word2, disassemble_info* info)
{
  int instr_len;

  instr_len = 4;
  put_abs (word1, word2, info);

  /* Divide instruction set into classes based on high 4 bits of opcode.  */
    switch (get_opcode_high4bits(word1)) {
        case 0x0:
        case 0x1:
            ctrl (memaddr, word1, word2, info);
            break;
        case 0x2:
        case 0x3:
            cobr (memaddr, word1, word2, info);
            break;
        case 0x5:
        case 0x6:
        case 0x7:
            reg (word1, info);
            break;
        case 0x8:
        case 0x9:
        case 0xa:
        case 0xb:
        case 0xc:
            instr_len = mem (memaddr, word1, word2, 0, info);
            break;
        default:
            /* Invalid instruction, print as data word.  */
            invalid (word1, info);
            break;
    }
    return instr_len;
}

/* CTRL format.. */

static void
ctrl (bfd_vma memaddr, uint32_t word1, uint32_t word2 ATTRIBUTE_UNUSED, disassemble_info* info)
{
  static const struct tabent ctrl_tab[] = {
    { NULL,		0, },	/* 0x00 */
    { NULL,		0, },	/* 0x01 */
    { NULL,		0, },	/* 0x02 */
    { NULL,		0, },	/* 0x03 */
    { NULL,		0, },	/* 0x04 */
    { NULL,		0, },	/* 0x05 */
    { NULL,		0, },	/* 0x06 */
    { NULL,		0, },	/* 0x07 */
    { "b",		1, },	/* 0x08 */
    { "call",		1, },	/* 0x09 */
    { "ret",		0, },	/* 0x0a */
    { "bal",		1, },	/* 0x0b */
    { NULL,		0, },	/* 0x0c */
    { NULL,		0, },	/* 0x0d */
    { NULL,		0, },	/* 0x0e */
    { NULL,		0, },	/* 0x0f */
    { "bno",		1, },	/* 0x10 */
    { "bg",		1, },	/* 0x11 */
    { "be",		1, },	/* 0x12 */
    { "bge",		1, },	/* 0x13 */
    { "bl",		1, },	/* 0x14 */
    { "bne",		1, },	/* 0x15 */
    { "ble",		1, },	/* 0x16 */
    { "bo",		1, },	/* 0x17 */
    { "faultno",	0, },	/* 0x18 */
    { "faultg",		0, },	/* 0x19 */
    { "faulte",		0, },	/* 0x1a */
    { "faultge",	0, },	/* 0x1b */
    { "faultl",		0, },	/* 0x1c */
    { "faultne",	0, },	/* 0x1d */
    { "faultle",	0, },	/* 0x1e */
    { "faulto",		0, },	/* 0x1f */
  };

  int i = (word1 >> 24) & 0xff;
  if ((ctrl_tab[i].name == NULL) || ((word1 & 1) != 0))
    {
      invalid (word1, info);
      return;
    }

  (*info->fprintf_func) (info->stream, "%s", ctrl_tab[i].name);
  if (word1 & 2)
    /* Predicts branch not taken.  */
    (*info->fprintf_func) (info->stream, ".f");

  if (ctrl_tab[i].numops == 1)
    {
      /* Extract displacement and convert to address.  */
      word1 &= 0x00ffffff;

      if (word1 & 0x00800000)
	{
	  /* Sign bit is set.  */
	  word1 |= (-1 & ~0xffffff);	/* Sign extend.  */
	}

      (*info->fprintf_func) (info->stream, "\t");
      print_addr (word1 + memaddr, info);
    }
}

/* COBR format.  */

static void
cobr (bfd_vma memaddr, uint32_t word1, uint32_t word2 ATTRIBUTE_UNUSED, disassemble_info* info)
{
  int src1;
  int src2;
  int i;

  static const struct tabent cobr_tab[] = {
    { "testno",	1, },	/* 0x20 */
    { "testg",	1, },	/* 0x21 */
    { "teste",	1, },	/* 0x22 */
    { "testge",	1, },	/* 0x23 */
    { "testl",	1, },	/* 0x24 */
    { "testne",	1, },	/* 0x25 */
    { "testle",	1, },	/* 0x26 */
    { "testo",	1, },	/* 0x27 */
    { NULL,	0, },	/* 0x28 */
    { NULL,	0, },	/* 0x29 */
    { NULL,	0, },	/* 0x2a */
    { NULL,	0, },	/* 0x2b */
    { NULL,	0, },	/* 0x2c */
    { NULL,	0, },	/* 0x2d */
    { NULL,	0, },	/* 0x2e */
    { NULL,	0, },	/* 0x2f */
    { "bbc",	3, },	/* 0x30 */
    { "cmpobg",	3, },	/* 0x31 */
    { "cmpobe",	3, },	/* 0x32 */
    { "cmpobge",3, },	/* 0x33 */
    { "cmpobl",	3, },	/* 0x34 */
    { "cmpobne",3, },	/* 0x35 */
    { "cmpoble",3, },	/* 0x36 */
    { "bbs",	3, },	/* 0x37 */
    { "cmpibno",3, },	/* 0x38 */
    { "cmpibg",	3, },	/* 0x39 */
    { "cmpibe",	3, },	/* 0x3a */
    { "cmpibge",3, },	/* 0x3b */
    { "cmpibl",	3, },	/* 0x3c */
    { "cmpibne",3, },	/* 0x3d */
    { "cmpible",3, },	/* 0x3e */
    { "cmpibo",	3, },	/* 0x3f */
  };

  i = ((word1 >> 24) & 0xff) - 0x20;
  if (cobr_tab[i].name == NULL)
    {
      invalid (word1, info);
      return;
    }

  (*info->fprintf_func) (info->stream, "%s", cobr_tab[i].name);

  /* Predicts branch not taken.  */
  if (word1 & 2)
    (*info->fprintf_func) (info->stream, ".f");

  (*info->fprintf_func) (info->stream, "\t");

  src1 = (word1 >> 19) & 0x1f;
  src2 = (word1 >> 14) & 0x1f;

  if (word1 & 0x02000)
    /* M1 is 1 */
    (*info->fprintf_func) (info->stream, "%d", src1);
  else
    (*info->fprintf_func) (info->stream, "%s", reg_names[src1]);

  if (cobr_tab[i].numops > 1)
    {
      if (word1 & 1)
	/* S2 is 1.  */
	(*info->fprintf_func) (info->stream, ",sf%d,", src2);
      else
	/* S1 is 0.  */
	(*info->fprintf_func) (info->stream, ",%s,", reg_names[src2]);

      /* Extract displacement and convert to address.  */
      word1 &= 0x00001ffc;
      if (word1 & 0x00001000)
	/* Negative displacement.  */
	word1 |= (-1 & ~0x1fff);	/* Sign extend.  */

      print_addr (memaddr + word1, info);
    }
}

/* MEM format.  */
/* Returns instruction length: 4 or 8.  */

static int
mem (bfd_vma memaddr, uint32_t word1, uint32_t word2, int noprint, disassemble_info* info)
{
  int i, j;
  int len;
  int mode;
  int offset;
  const char *reg1, *reg2, *reg3;

  /* This lookup table is too sparse to make it worth typing in, but not
     so large as to make a sparse array necessary.  We create the table
     at runtime.  */

  /* NOTE: In this table, the meaning of 'numops' is:
      1: single operand
      2: 2 operands, load instruction
     -2: 2 operands, store instruction.  */
  static struct tabent *mem_tab;
  /* Opcodes of 0x8X, 9X, aX, bX, and cX must be in the table.  */
#define MEM_MIN	0x80
#define MEM_MAX	0xcf
#define MEM_SIZ	( * sizeof(struct tabent))

  static const struct sparse_tabent mem_init[] = {
    { 0x80,	"ldob",	 2 },
    { 0x82,	"stob",	-2 },
    { 0x84,	"bx",	 1 },
    { 0x85,	"balx",	 2 },
    { 0x86,	"callx", 1 },
    { 0x88,	"ldos",	 2 },
    { 0x8a,	"stos",	-2 },
    { 0x8c,	"lda",	 2 },
    { 0x90,	"ld",	 2 },
    { 0x92,	"st",	-2 },
    { 0x98,	"ldl",	 2 },
    { 0x9a,	"stl",	-2 },
    { 0xa0,	"ldt",	 2 },
    { 0xa2,	"stt",	-2 },
    { 0xac,	"dcinva", 1 },
    { 0xb0,	"ldq",	 2 },
    { 0xb2,	"stq",	-2 },
    { 0xc0,	"ldib",	 2 },
    { 0xc2,	"stib",	-2 },
    { 0xc8,	"ldis",	 2 },
    { 0xca,	"stis",	-2 },
    { 0,	NULL,	0 }
  };
  static struct tabent mem_tab_buf[MEM_MAX - MEM_MIN + 1];

  if (mem_tab == NULL)
    {
      mem_tab = mem_tab_buf;

      for (i = 0; mem_init[i].opcode != 0; i++)
	{
	  j = mem_init[i].opcode - MEM_MIN;
	  mem_tab[j].name = mem_init[i].name;
	  mem_tab[j].numops = mem_init[i].numops;
	}
    }

  i = ((word1 >> 24) & 0xff) - MEM_MIN;
  mode = (word1 >> 10) & 0xf;

  if ((mem_tab[i].name != NULL)		/* Valid instruction */
      && ((mode == 5) || (mode >= 12)))
    /* With 32-bit displacement.  */
    len = 8;
  else
    len = 4;

  if (noprint)
    return len;

  if ((mem_tab[i].name == NULL) || (mode == 6))
    {
      invalid (word1, info);
      return len;
    }

  (*info->fprintf_func) (info->stream, "%s\t", mem_tab[i].name);

  reg1 = reg_names[ (word1 >> 19) & 0x1f ];	/* MEMB only */
  reg2 = reg_names[ (word1 >> 14) & 0x1f ];
  reg3 = reg_names[ word1 & 0x1f ];		/* MEMB only */
  offset = word1 & 0xfff;				/* MEMA only  */

  switch (mem_tab[i].numops)
    {
    case 2: /* LOAD INSTRUCTION */
      if (mode & 4)
	{			/* MEMB FORMAT */
	  ea (memaddr, mode, reg2, reg3, word1, word2, info);
	  (*info->fprintf_func) (info->stream, ",%s", reg1);
	}
      else
	{				/* MEMA FORMAT */
	  (*info->fprintf_func) (info->stream, "0x%x", (unsigned) offset);

	  if (mode & 8)
	    (*info->fprintf_func) (info->stream, "(%s)", reg2);

	  (*info->fprintf_func)(info->stream, ",%s", reg1);
	}
      break;

    case -2: /* STORE INSTRUCTION */
      if (mode & 4)
	{
	  /* MEMB FORMAT */
	  (*info->fprintf_func) (info->stream, "%s,", reg1);
	  ea (memaddr, mode, reg2, reg3, word1, word2, info);
	}
      else
	{
	  /* MEMA FORMAT */
	  (*info->fprintf_func) (info->stream, "%s,0x%x", reg1, (unsigned) offset);

	  if (mode & 8)
	    (*info->fprintf_func) (info->stream, "(%s)", reg2);
	}
      break;

    case 1: /* BX/CALLX INSTRUCTION */
      if (mode & 4)
	{
	  /* MEMB FORMAT */
	  ea (memaddr, mode, reg2, reg3, word1, word2, info);
	}
      else
	{
	  /* MEMA FORMAT */
	  (*info->fprintf_func) (info->stream, "0x%x", (unsigned) offset);
	  if (mode & 8)
	    (*info->fprintf_func) (info->stream, "(%s)", reg2);
	}
      break;
    }

  return len;
}

/* REG format.  */

static void
reg (uint32_t word1, disassemble_info* info)
{
  int i, j;
  int opcode;
  int fp;
  int m1, m2, m3;
  int s1, s2;
  int src, src2, dst;
  char *mnemp;

  /* This lookup table is too sparse to make it worth typing in, but not
     so large as to make a sparse array necessary.  We create the table
     at runtime.  */

  /* NOTE: In this table, the meaning of 'numops' is:
	 1: single operand, which is NOT a destination.
	-1: single operand, which IS a destination.
	 2: 2 operands, the 2nd of which is NOT a destination.
	-2: 2 operands, the 2nd of which IS a destination.
	 3: 3 operands

	If an opcode mnemonic begins with "F", it is a floating-point
	opcode (the "F" is not printed).  */

  static struct tabent *reg_tab;
  static const struct sparse_tabent reg_init[] =
  {
#define REG_MIN	0x580
    { 0x580,	"notbit",	3 },
    { 0x581,	"and",		3 },
    { 0x582,	"andnot",	3 },
    { 0x583,	"setbit",	3 },
    { 0x584,	"notand",	3 },
    { 0x586,	"xor",		3 },
    { 0x587,	"or",		3 },
    { 0x588,	"nor",		3 },
    { 0x589,	"xnor",		3 },
    { 0x58a,	"not",		-2 },
    { 0x58b,	"ornot",	3 },
    { 0x58c,	"clrbit",	3 },
    { 0x58d,	"notor",	3 },
    { 0x58e,	"nand",		3 },
    { 0x58f,	"alterbit",	3 },
    { 0x590,	"addo",		3 },
    { 0x591,	"addi",		3 },
    { 0x592,	"subo",		3 },
    { 0x593,	"subi",		3 },
    { 0x594,	"cmpob",	2 },
    { 0x595,	"cmpib",	2 },
    { 0x596,	"cmpos",	2 },
    { 0x597,	"cmpis",	2 },
    { 0x598,	"shro",		3 },
    { 0x59a,	"shrdi",	3 },
    { 0x59b,	"shri",		3 },
    { 0x59c,	"shlo",		3 },
    { 0x59d,	"rotate",	3 },
    { 0x59e,	"shli",		3 },
    { 0x5a0,	"cmpo",		2 },
    { 0x5a1,	"cmpi",		2 },
    { 0x5a2,	"concmpo",	2 },
    { 0x5a3,	"concmpi",	2 },
    { 0x5a4,	"cmpinco",	3 },
    { 0x5a5,	"cmpinci",	3 },
    { 0x5a6,	"cmpdeco",	3 },
    { 0x5a7,	"cmpdeci",	3 },
    { 0x5ac,	"scanbyte",	2 },
    { 0x5ad,	"bswap",	-2 },
    { 0x5ae,	"chkbit",	2 },
    { 0x5b0,	"addc",		3 },
    { 0x5b2,	"subc",		3 },
    { 0x5b4,	"intdis",	0 },
    { 0x5b5,	"inten",	0 },
    { 0x5cc,	"mov",		-2 },
    { 0x5d8,	"eshro",	3 },
    { 0x5dc,	"movl",		-2 },
    { 0x5ec,	"movt",		-2 },
    { 0x5fc,	"movq",		-2 },
    { 0x600,	"synmov",	2 },
    { 0x601,	"synmovl",	2 },
    { 0x602,	"synmovq",	2 },
    { 0x603,	"cmpstr",	3 },
    { 0x604,	"movqstr",	3 },
    { 0x605,	"movstr",	3 },
    { 0x610,	"atmod",	3 },
    { 0x612,	"atadd",	3 },
    { 0x613,	"inspacc",	-2 },
    { 0x614,	"ldphy",	-2 },
    { 0x615,	"synld",	-2 },
    { 0x617,	"fill",		3 },
    { 0x630,	"sdma",		3 },
    { 0x631,	"udma",		0 },
    { 0x640,	"spanbit",	-2 },
    { 0x641,	"scanbit",	-2 },
    { 0x642,	"daddc",	3 },
    { 0x643,	"dsubc",	3 },
    { 0x644,	"dmovt",	-2 },
    { 0x645,	"modac",	3 },
    { 0x646,	"condrec",	-2 },
    { 0x650,	"modify",	3 },
    { 0x651,	"extract",	3 },
    { 0x654,	"modtc",	3 },
    { 0x655,	"modpc",	3 },
    { 0x656,	"receive",	-2 },
    { 0x658,	"intctl",	-2 },
    { 0x659,	"sysctl",	3 },
    { 0x65b,	"icctl",	3 },
    { 0x65c,	"dcctl",	3 },
    { 0x65d,	"halt",		0 },
    { 0x660,	"calls",	1 },
    { 0x662,	"send",		3 },
    { 0x663,	"sendserv",	1 },
    { 0x664,	"resumprcs",	1 },
    { 0x665,	"schedprcs",	1 },
    { 0x666,	"saveprcs",	0 },
    { 0x668,	"condwait",	1 },
    { 0x669,	"wait",		1 },
    { 0x66a,	"signal",	1 },
    { 0x66b,	"mark",		0 },
    { 0x66c,	"fmark",	0 },
    { 0x66d,	"flushreg",	0 },
    { 0x66f,	"syncf",	0 },
    { 0x670,	"emul",		3 },
    { 0x671,	"ediv",		3 },
    { 0x673,	"ldtime",	-1 },
    { 0x674,	"Fcvtir",	-2 },
    { 0x675,	"Fcvtilr",	-2 },
    { 0x676,	"Fscalerl",	3 },
    { 0x677,	"Fscaler",	3 },
    { 0x680,	"Fatanr",	3 },
    { 0x681,	"Flogepr",	3 },
    { 0x682,	"Flogr",	3 },
    { 0x683,	"Fremr",	3 },
    { 0x684,	"Fcmpor",	2 },
    { 0x685,	"Fcmpr",	2 },
    { 0x688,	"Fsqrtr",	-2 },
    { 0x689,	"Fexpr",	-2 },
    { 0x68a,	"Flogbnr",	-2 },
    { 0x68b,	"Froundr",	-2 },
    { 0x68c,	"Fsinr",	-2 },
    { 0x68d,	"Fcosr",	-2 },
    { 0x68e,	"Ftanr",	-2 },
    { 0x68f,	"Fclassr",	1 },
    { 0x690,	"Fatanrl",	3 },
    { 0x691,	"Flogeprl",	3 },
    { 0x692,	"Flogrl",	3 },
    { 0x693,	"Fremrl",	3 },
    { 0x694,	"Fcmporl",	2 },
    { 0x695,	"Fcmprl",	2 },
    { 0x698,	"Fsqrtrl",	-2 },
    { 0x699,	"Fexprl",	-2 },
    { 0x69a,	"Flogbnrl",	-2 },
    { 0x69b,	"Froundrl",	-2 },
    { 0x69c,	"Fsinrl",	-2 },
    { 0x69d,	"Fcosrl",	-2 },
    { 0x69e,	"Ftanrl",	-2 },
    { 0x69f,	"Fclassrl",	1 },
    { 0x6c0,	"Fcvtri",	-2 },
    { 0x6c1,	"Fcvtril",	-2 },
    { 0x6c2,	"Fcvtzri",	-2 },
    { 0x6c3,	"Fcvtzril",	-2 },
    { 0x6c9,	"Fmovr",	-2 },
    { 0x6d9,	"Fmovrl",	-2 },
    /* the manuals are flat out WRONG! They state 0x6e9 as the opcode not 0x6e1 */
    { 0x6e1,	"Fmovre",	-2 }, 
    { 0x6e2,	"Fcpysre",	3 },
    { 0x6e3,	"Fcpyrsre",	3 },
    { 0x701,	"mulo",		3 },
    { 0x708,	"remo",		3 },
    { 0x70b,	"divo",		3 },
    { 0x741,	"muli",		3 },
    { 0x748,	"remi",		3 },
    { 0x749,	"modi",		3 },
    { 0x74b,	"divi",		3 },
    { 0x780,	"addono",	3 },
    { 0x781,	"addino",	3 },
    { 0x782,	"subono",	3 },
    { 0x783,	"subino",	3 },
    { 0x784,	"selno",	3 },
    { 0x78b,	"Fdivr",	3 },
    { 0x78c,	"Fmulr",	3 },
    { 0x78d,	"Fsubr",	3 },
    { 0x78f,	"Faddr",	3 },
    { 0x790,	"addog",	3 },
    { 0x791,	"addig",        3 },
    { 0x792,	"subog",	3 },
    { 0x793,	"subig",	3 },
    { 0x794,	"selg",		3 },
    { 0x79b,	"Fdivrl",	3 },
    { 0x79c,	"Fmulrl",	3 },
    { 0x79d,	"Fsubrl",	3 },
    { 0x79f,	"Faddrl",	3 },
    { 0x7a0,	"addoe",	3 },
    { 0x7a1,	"addie",        3 },
    { 0x7a2,	"suboe",	3 },
    { 0x7a3,	"subie",	3 },
    { 0x7a4,	"sele",		3 },
    { 0x7b0,	"addoge",	3 },
    { 0x7b1,	"addige",	3 },
    { 0x7b2,	"suboge",	3 },
    { 0x7b3,	"subige",	3 },
    { 0x7b4,	"selge",	3 },
    { 0x7c0,	"addol",	3 },
    { 0x7c1,	"addil",	3 },
    { 0x7c2,	"subol",	3 },
    { 0x7c3,	"subil",	3 },
    { 0x7c4,	"sell",		3 },
    { 0x7d0,	"addone",	3 },
    { 0x7d1,	"addine",	3 },
    { 0x7d2,	"subone",	3 },
    { 0x7d3,	"subine",	3 },
    { 0x7d4,	"selne",	3 },
    { 0x7e0,	"addole",	3 },
    { 0x7e1,	"addile",	3 },
    { 0x7e2,	"subole",	3 },
    { 0x7e3,	"subile",	3 },
    { 0x7e4,	"selle",	3 },
    { 0x7f0,	"addoo",	3 },
    { 0x7f1,	"addio",	3 },
    { 0x7f2,	"suboo",	3 },
    { 0x7f3,	"subio",	3 },
    { 0x7f4,	"selo",		3 },
#define REG_MAX 0x7f4
    { 0,	NULL,		0 }
  };
  static struct tabent reg_tab_buf[REG_MAX - REG_MIN + 1];

  if (reg_tab == NULL)
    {
      reg_tab = reg_tab_buf;

      for (i = 0; reg_init[i].opcode != 0; i++)
	{
	  j = reg_init[i].opcode - REG_MIN;
	  reg_tab[j].name = reg_init[i].name;
	  reg_tab[j].numops = reg_init[i].numops;
	}
    }

  opcode = ((word1 >> 20) & 0xff0) | ((word1 >> 7) & 0xf);
  i = opcode - REG_MIN;

  if ((opcode<REG_MIN) || (opcode>REG_MAX) || (reg_tab[i].name==NULL))
    {
      invalid (word1, info);
      return;
    }

  mnemp = reg_tab[i].name;
  if (*mnemp == 'F')
    {
      fp = 1;
      mnemp++;
    }
  else
    {
      fp = 0;
    }

  (*info->fprintf_func) (info->stream, "%s", mnemp);

  s1   = (word1 >> 5)  & 1;
  s2   = (word1 >> 6)  & 1;
  m1   = (word1 >> 11) & 1;
  m2   = (word1 >> 12) & 1;
  m3   = (word1 >> 13) & 1;
  src  =  word1        & 0x1f;
  src2 = (word1 >> 14) & 0x1f;
  dst  = (word1 >> 19) & 0x1f;

  if  (reg_tab[i].numops != 0)
    {
      (*info->fprintf_func) (info->stream, "\t");

    switch (reg_tab[i].numops)
      {
      case 1:
	regop (m1, s1, src, fp, info);
	break;
      case -1:
	dstop (m3, dst, fp, info);
	break;
      case 2:
	regop (m1, s1, src, fp, info);
	(*info->fprintf_func) (info->stream, ",");
	regop (m2, s2, src2, fp, info);
	break;
      case -2:
	regop (m1, s1, src, fp, info);
	(*info->fprintf_func) (info->stream, ",");
	dstop (m3, dst, fp, info);
	break;
      case 3:
	regop (m1, s1, src, fp, info);
	(*info->fprintf_func) (info->stream, ",");
	regop (m2, s2, src2, fp, info);
	(*info->fprintf_func) (info->stream, ",");
	dstop (m3, dst, fp, info);
	break;
      }
    }
}

/* Print out effective address for memb instructions.  */

static void
ea (bfd_vma memaddr, int mode, const char *reg2, const char *reg3, int word1,
    unsigned int word2, disassemble_info* info)
{
  int scale;
  static const int scale_tab[] = { 1, 2, 4, 8, 16 };

  scale = (word1 >> 7) & 0x07;

  if ((scale > 4) || (((word1 >> 5) & 0x03) != 0))
    {
      invalid (word1, info);
      return;
    }
  scale = scale_tab[scale];

  switch (mode)
    {
    case 4:						/* (reg) */
      (*info->fprintf_func)( info->stream, "(%s)", reg2 );
      break;
    case 5:						/* displ+8(ip) */
      print_addr (word2 + 8 + memaddr, info);
      break;
    case 7:						/* (reg)[index*scale] */
      if (scale == 1)
	(*info->fprintf_func) (info->stream, "(%s)[%s]", reg2, reg3);
      else
	(*info->fprintf_func) (info->stream, "(%s)[%s*%d]", reg2, reg3, scale);
      break;
    case 12:					/* displacement */
      print_addr ((bfd_vma) word2, info);
      break;
    case 13:					/* displ(reg) */
      print_addr ((bfd_vma) word2, info);
      (*info->fprintf_func) (info->stream, "(%s)", reg2);
      break;
    case 14:					/* displ[index*scale] */
      print_addr ((bfd_vma) word2, info);
      if (scale == 1)
	(*info->fprintf_func) (info->stream, "[%s]", reg3);
      else
	(*info->fprintf_func) (info->stream, "[%s*%d]", reg3, scale);
      break;
    case 15:				/* displ(reg)[index*scale] */
      print_addr ((bfd_vma) word2, info);
      if (scale == 1)
	(*info->fprintf_func) (info->stream, "(%s)[%s]", reg2, reg3);
      else
	(*info->fprintf_func) (info->stream, "(%s)[%s*%d]", reg2, reg3, scale);
      break;
    default:
      invalid (word1, info);
      return;
    }
}


/* Register Instruction Operand.  */

static void
regop (int mode, int spec, int fp_reg, int fp, disassemble_info* info)
{
  if (fp)
    {
      /* Floating point instruction.  */
      if (mode == 1)
	{
	  /* FP operand.  */
	  switch (fp_reg)
	    {
	    case 0:  (*info->fprintf_func) (info->stream, "fp0");
	      break;
	    case 1:  (*info->fprintf_func) (info->stream, "fp1");
	      break;
	    case 2:  (*info->fprintf_func) (info->stream, "fp2");
	      break;
	    case 3:  (*info->fprintf_func) (info->stream, "fp3");
	      break;
	    case 16: (*info->fprintf_func) (info->stream, "0f0.0");
	      break;
	    case 22: (*info->fprintf_func) (info->stream, "0f1.0");
	      break;
	    default: (*info->fprintf_func) (info->stream, "?");
	      break;
	    }
	}
      else
	{
	  /* Non-FP register.  */
	  (*info->fprintf_func) (info->stream, "%s", reg_names[fp_reg]);
	}
    }
  else
    {
      /* Not floating point.  */
      if (mode == 1)
	{
	  /* Literal.  */
	  (*info->fprintf_func) (info->stream, "%d", fp_reg);
	}
      else
	{
	  /* Register.  */
	  if (spec == 0)
	    (*info->fprintf_func) (info->stream, "%s", reg_names[fp_reg]);
	  else
	    (*info->fprintf_func) (info->stream, "sf%d", fp_reg);
	}
    }
}

/* Register Instruction Destination Operand.  */

static void
dstop (int mode, int dest_reg, int fp, disassemble_info* info)
{
  /* 'dst' operand can't be a literal. On non-FP instructions,  register
     mode is assumed and "m3" acts as if were "s3";  on FP-instructions,
     sf registers are not allowed so m3 acts normally.  */
  if (fp)
    regop (mode, 0, dest_reg, fp, info);
  else
    regop (0, mode, dest_reg, fp, info);
}

static void
invalid (int word1, disassemble_info* info)
{
  (*info->fprintf_func) (info->stream, ".word\t0x%08x", (unsigned) word1);
}

static void
print_addr (bfd_vma a, disassemble_info* info)
{
  (*info->print_address_func) (a, info);
}

static void
put_abs (uint32_t word1,
	 uint32_t word2,
     disassemble_info* info)
{
  int len;

  switch (get_opcode_high4bits(word1))
    {
    case 0x8:
    case 0x9:
    case 0xa:
    case 0xb:
    case 0xc:
      /* MEM format instruction.  */
      len = mem (0, word1, word2, 1, info);
      break;
    default:
      len = 4;
      break;
    }

  if (len == 8)
    (*info->fprintf_func) (info->stream, "%08x %08x\t", word1, word2);
  else
    (*info->fprintf_func) (info->stream, "%08x         \t", word1);
}
