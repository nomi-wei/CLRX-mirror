## GCN ISA VOP1/VOP3 instructions

VOP1 instructions can be encoded in the VOP1 encoding and the VOP3A/VOP3B encoding.
List of fields for VOP1 encoding:

Bits  | Name     | Description
------|----------|------------------------------
0-8   | SRC0     | First (scalar or vector) source operand
9-16  | OPCODE   | Operation code
17-24 | VDST     | Destination vector operand
25-31 | ENCODING | Encoding type. Must be 0b0111111

Syntax: INSTRUCTION VDST, SRC0

List of fields for VOP3A/VOP3B encoding (GCN 1.0/1.1):

Bits  | Name     | Description
------|----------|------------------------------
0-7   | VDST     | Vector destination operand
8-10  | ABS      | Absolute modifiers for source operands (VOP3A)
8-14  | SDST     | Scalar destination operand (VOP3B)
11    | CLAMP    | CLAMP modifier (VOP3A)
15    | CLAMP    | CLAMP modifier (VOP3B)
17-25 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b110100
32-40 | SRC0     | First (scalar or vector) source operand
41-49 | SRC1     | Second (scalar or vector) source operand
50-58 | SRC2     | Third (scalar or vector) source operand
59-60 | OMOD     | OMOD modifier. Multiplication modifier
61-63 | NEG      | Negation modifier for source operands

List of fields for VOP3A/VOP3B encoding (GCN 1.2):

Bits  | Name     | Description
------|----------|------------------------------
0-7   | VDST     | Destination vector operand
8-10  | ABS      | Absolute modifiers for source operands (VOP3A)
8-14  | SDST     | Scalar destination operand (VOP3B)
15    | CLAMP    | CLAMP modifier
16-25 | OPCODE   | Operation code
26-31 | ENCODING | Encoding type. Must be 0b110100
32-40 | SRC0     | First (scalar or vector) source operand
41-49 | SRC1     | Second (scalar or vector) source operand
50-58 | SRC2     | Third (scalar or vector) source operand
59-60 | OMOD     | OMOD modifier. Multiplication modifier
61-63 | NEG      | Negation modifier for source operands

Syntax: INSTRUCTION VDST, SRC0 [MODIFIERS]

Modifiers:

* CLAMP - clamps destination floating point value in range 0.0-1.0
* MUL:2, MUL:4, DIV:2 - OMOD modifiers. Multiply destination floating point value by
2.0, 4.0 or 0.5 respectively
* -SRC - negate floating point value from source operand
* ABS(SRC) - apply absolute value to source operand

Negation and absolute value can be combined: `-ABS(V0)`. Modifiers CLAMP and
OMOD (MUL:2, MUL:4 and DIV:2) can be given in random order.

Limitations for operands:

* only one SGPR can be read by instruction. Multiple occurrences of this same
SGPR is allowed
* only one literal constant can be used, and only when a SGPR or M0 is not used in
source operands
* only SRC0 can holds LDS_DIRECT

VOP1 opcodes (0-127) are reflected in VOP3 in range: 384-511 for GCN 1.0/1.1 or
320-447 for GCN 1.2.

List of the instructions by opcode (GCN 1.0/1.1):

 Opcode     | Opcode(VOP3)|GCN 1.0|GCN 1.1| Mnemonic
------------|-------------|-------|-------|-----------------------------
 0 (0x0)    | 384 (0x180) |   ✓   |   ✓   | V_NOP
 1 (0x1)    | 385 (0x181) |   ✓   |   ✓   | V_MOV_B32
 2 (0x2)    | 386 (0x182) |   ✓   |   ✓   | V_READFIRSTLANE_B32
 3 (0x3)    | 387 (0x183) |   ✓   |   ✓   | V_CVT_I32_F64
 4 (0x4)    | 388 (0x184) |   ✓   |   ✓   | V_CVT_F64_I32
 5 (0x5)    | 389 (0x185) |   ✓   |   ✓   | V_CVT_F32_I32
 6 (0x6)    | 390 (0x186) |   ✓   |   ✓   | V_CVT_F32_U32
 7 (0x7)    | 391 (0x187) |   ✓   |   ✓   | V_CVT_U32_F32
 8 (0x8)    | 392 (0x188) |   ✓   |   ✓   | V_CVT_I32_F32
 9 (0x9)    | 393 (0x189) |   ✓   |   ✓   | V_MOV_FED_B32
 10 (0xa)   | 394 (0x18a) |   ✓   |   ✓   | V_CVT_F16_F32
 11 (0xb)   | 395 (0x18b) |   ✓   |   ✓   | V_CVT_F32_F16
 12 (0xc)   | 396 (0x18c) |   ✓   |   ✓   | V_CVT_RPI_I32_F32
 13 (0xd)   | 397 (0x18d) |   ✓   |   ✓   | V_CVT_FLR_I32_F32
 14 (0xe)   | 398 (0x18e) |   ✓   |   ✓   | V_CVT_OFF_F32_I4
 15 (0xf)   | 399 (0x18f) |   ✓   |   ✓   | V_CVT_F32_F64
 16 (0x10)  | 400 (0x190) |   ✓   |   ✓   | V_CVT_F64_F32
 17 (0x11)  | 401 (0x191) |   ✓   |   ✓   | V_CVT_F32_UBYTE0
 18 (0x12)  | 402 (0x192) |   ✓   |   ✓   | V_CVT_F32_UBYTE1
 19 (0x13)  | 403 (0x193) |   ✓   |   ✓   | V_CVT_F32_UBYTE2
 20 (0x14)  | 404 (0x194) |   ✓   |   ✓   | V_CVT_F32_UBYTE3
 21 (0x15)  | 405 (0x195) |   ✓   |   ✓   | V_CVT_U32_F64
 22 (0x16)  | 406 (0x196) |   ✓   |   ✓   | V_CVT_F64_U32
 23 (0x17)  | 407 (0x197) |   ✓   |   ✓   | V_TRUNC_F64
 24 (0x18)  | 408 (0x198) |   ✓   |   ✓   | V_CEIL_F64
 25 (0x19)  | 409 (0x199) |   ✓   |   ✓   | V_RNDNE_F64
 26 (0x1a)  | 410 (0x19a) |   ✓   |   ✓   | V_FLOOR_F64
 32 (0x20)  | 416 (0x1a0) |   ✓   |   ✓   | V_FRACT_F32
 33 (0x21)  | 417 (0x1a1) |   ✓   |   ✓   | V_TRUNC_F32
 34 (0x22)  | 418 (0x1a2) |   ✓   |   ✓   | V_CEIL_F32
 35 (0x23)  | 419 (0x1a3) |   ✓   |   ✓   | V_RNDNE_F32
 36 (0x24)  | 420 (0x1a4) |   ✓   |   ✓   | V_FLOOR_F32
 37 (0x25)  | 421 (0x1a5) |   ✓   |   ✓   | V_EXP_F32
 38 (0x26)  | 422 (0x1a6) |   ✓   |   ✓   | V_LOG_CLAMP_F32
 39 (0x27)  | 423 (0x1a7) |   ✓   |   ✓   | V_LOG_F32
 40 (0x28)  | 424 (0x1a8) |   ✓   |   ✓   | V_RCP_CLAMP_F32
 41 (0x29)  | 425 (0x1a9) |   ✓   |   ✓   | V_RCP_LEGACY_F32
 42 (0x2a)  | 426 (0x1aa) |   ✓   |   ✓   | V_RCP_F32
 43 (0x2b)  | 427 (0x1ab) |   ✓   |   ✓   | V_RCP_IFLAG_F32
 44 (0x2c)  | 428 (0x1ac) |   ✓   |   ✓   | V_RSQ_CLAMP_F32
 45 (0x2d)  | 429 (0x1ad) |   ✓   |   ✓   | V_RSQ_LEGACY_F32
 46 (0x2e)  | 430 (0x1ae) |   ✓   |   ✓   | V_RSQ_F32
 47 (0x2f)  | 431 (0x1af) |   ✓   |   ✓   | V_RCP_F64
 48 (0x30)  | 432 (0x1b0) |   ✓   |   ✓   | V_RCP_CLAMP_F64
 49 (0x31)  | 433 (0x1b1) |   ✓   |   ✓   | V_RSQ_F64
 50 (0x32)  | 434 (0x1b2) |   ✓   |   ✓   | V_RSQ_CLAMP_F64
 51 (0x33)  | 435 (0x1b3) |   ✓   |   ✓   | V_SQRT_F32
 52 (0x34)  | 436 (0x1b4) |   ✓   |   ✓   | V_SQRT_F64
 53 (0x35)  | 437 (0x1b5) |   ✓   |   ✓   | V_SIN_F32
 54 (0x36)  | 438 (0x1b6) |   ✓   |   ✓   | V_COS_F32
 55 (0x37)  | 439 (0x1b7) |   ✓   |   ✓   | V_NOT_B32
 56 (0x38)  | 440 (0x1b8) |   ✓   |   ✓   | V_BFREV_B32
 57 (0x39)  | 441 (0x1b9) |   ✓   |   ✓   | V_FFBH_U32
 58 (0x3a)  | 442 (0x1ba) |   ✓   |   ✓   | V_FFBL_B32
 59 (0x3b)  | 443 (0x1bb) |   ✓   |   ✓   | V_FFBH_I32
 60 (0x3c)  | 444 (0x1bc) |   ✓   |   ✓   | V_FREXP_EXP_I32_F64
 61 (0x3d)  | 445 (0x1bd) |   ✓   |   ✓   | V_FREXP_MANT_F64
 62 (0x3e)  | 446 (0x1be) |   ✓   |   ✓   | V_FRACT_F64
 63 (0x3f)  | 447 (0x1bf) |   ✓   |   ✓   | V_FREXP_EXP_I32_F32
 64 (0x40)  | 448 (0x1c0) |   ✓   |   ✓   | V_FREXP_MANT_F32
 65 (0x41)  | 449 (0x1c1) |   ✓   |   ✓   | V_CLREXCP
 66 (0x42)  | 450 (0x1c2) |   ✓   |   ✓   | V_MOVRELD_B32
 67 (0x43)  | 451 (0x1c3) |   ✓   |   ✓   | V_MOVRELS_B32
 68 (0x44)  | 452 (0x1c4) |   ✓   |   ✓   | V_MOVRELSD_B32
 69 (0x45)  | 453 (0x1c5) |       |   ✓   | V_LOG_LEGACY_F32
 70 (0x46)  | 454 (0x1c6) |       |   ✓   | V_EXP_LEGACY_F32