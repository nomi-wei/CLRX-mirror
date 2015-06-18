/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2015 Mateusz Szpakowski
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <CLRX/Config.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <CLRX/amdasm/Disassembler.h>

using namespace CLRX;

static const cxbyte disasmInput1Global[14] =
{ 13, 56, 66, 213,
  55, 93, 123, 85,
  164, 234, 21, 37,
  44, 188 };

static const char* disasmInput1Kernel1Metadata =
";ARGSTART:__OpenCL_DCT_kernel\n"
";version:3:1:111\n"
";device:pitcairn\n"
";uniqueid:1024\n"
";memory:uavprivate:0\n"
";memory:hwlocal:0\n"
";memory:hwregion:0\n"
";pointer:output:float:1:1:0:uav:12:4:RW:0:0\n"
";pointer:input:float:1:1:16:uav:13:4:RO:0:0\n"
";pointer:dct8x8:float:1:1:32:uav:14:4:RO:0:0\n"
";pointer:inter:float:1:1:48:hl:1:4:RW:0:0\n"
";value:width:u32:1:1:64\n"
";value:blockWidth:u32:1:1:80\n"
";value:inverse:u32:1:1:96\n"
";function:1:1030\n"
";uavid:11\n"
";printfid:9\n"
";cbid:10\n"
";privateid:8\n"
";reflection:0:float*\n"
";reflection:1:float*\n"
";reflection:2:float*\n"
";reflection:3:float*\n"
";reflection:4:uint\n"
";reflection:5:uint\n"
";reflection:6:uint\n"
";ARGEND:__OpenCL_DCT_kernel\n"
"dessss9843re88888888888888888uuuuuuuufdd"
"dessss9843re88888888888888888uuuuuuuufdd"
"dessss9843re88888888888888888uuuuuuuufdd444444444444444444444444"
"\r\r\t\t\t\t\txx\f\f\f\3777x833334441\n";

static const cxbyte disasmInput1Kernel1Header[59] =
{ 1, 2, 0, 0, 0, 44, 0, 0,
  12, 3, 6, 3, 3, 2, 0, 0,
  19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 18,
  19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19, 19, 19, 19, 19, 19,
  19, 19, 19
};

static const cxbyte disasmInput1Kernel1Data[40] =
{
    1, 4, 5, 6, 6, 6, 4, 3,
    3, 5, 65, 13, 5, 5, 11, 57,
    5, 65, 5, 0, 0, 0, 0, 5,
    1, 15, 5, 1, 56, 0, 0, 5,
    5, 65, 5, 0, 0, 78, 255, 5
};

static uint32_t disasmInput1Kernel1ProgInfo[13] =
{
    0xffcd44dc, 0x4543,
    0x456, 0x5663677,
    0x3cd90c, 0x3958a85,
    0x458c98d9, 0x344dbd9,
    0xd0d9d9d, 0x234455,
    0x1, 0x55, 0x4565
};

static uint32_t disasmInput1Kernel1Inputs[20] =
{
    1113, 1113, 1113, 1113, 1113, 1113, 1113, 1113,
    1113, 1113, 1113, 1114, 1113, 1113, 1113, 1113,
    1113, 1113, 1113, 1113
};

static uint32_t disasmInput1Kernel1Outputs[17] =
{
    5, 6, 3, 13, 5, 117, 12, 3,
    785, 46, 55, 5, 2, 3, 0, 0, 44
};

static uint32_t disasmInput1Kernel1EarlyExit[3] = { 1355, 44, 444 };

static uint32_t disasmInput1Kernel2EarlyExit[3] = { 121 };

static CALDataSegmentEntry disasmInput1Kernel1Float32Consts[6] =
{
    { 0, 5 }, { 66, 57 }, { 67, 334 }, { 1, 6 }, { 5, 86 }, { 2100, 466 }
};

static CALSamplerMapEntry disasmInput1Kernel1InputSamplers[6] =
{
    { 0, 5 }, { 66, 57 }, { 67, 334 }, { 1, 6 }, { 5, 86 }, { 2100, 466 }
};

static uint32_t disasmInput1Kernel1UavOpMask[1] = { 4556 };

static AmdDisasmInput disasmInput1 =
{
    GPUDeviceType::SPOOKY,
    true,
    "This\nis my\300\x31 stupid\r\"driver", "-O ccc",
    sizeof(disasmInput1Global), disasmInput1Global,
    {
        { "kernelxVCR",
          ::strlen(disasmInput1Kernel1Metadata),
          disasmInput1Kernel1Metadata,
          sizeof(disasmInput1Kernel1Header), disasmInput1Kernel1Header,
          {
              { { 8, 52, CALNOTE_ATI_PROGINFO,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1ProgInfo) },
              { { 8, 79, CALNOTE_ATI_INPUTS,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1Inputs) },
              { { 8, 67, CALNOTE_ATI_OUTPUTS,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1Outputs) },
              { { 8, 12, CALNOTE_ATI_EARLYEXIT,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1EarlyExit) },
              { { 8, 4, CALNOTE_ATI_EARLYEXIT,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel2EarlyExit) },
              { { 8, 48, CALNOTE_ATI_FLOAT32CONSTS,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1Float32Consts) },
              { { 8, 48, CALNOTE_ATI_INPUT_SAMPLERS,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1InputSamplers) },
              { { 8, 48, CALNOTE_ATI_CONSTANT_BUFFERS,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1InputSamplers) },
              { { 8, 48, 0x3d, { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1InputSamplers) },
              { { 8, 4, CALNOTE_ATI_UAV_OP_MASK,
                  { 'A', 'T', 'I', ' ', 'C', 'A', 'L', 0 } },
                 reinterpret_cast<cxbyte*>(disasmInput1Kernel1UavOpMask) },
          },
          sizeof(disasmInput1Kernel1Data), disasmInput1Kernel1Data,
          0, nullptr
        }
    }
};

struct DisasmDataTestCase
{
    const AmdDisasmInput* amdInput;
    const char* expectedString;
};

static const DisasmDataTestCase disasmDataTestCases[] =
{
    { &disasmInput1,
        R"xxFxx(.amd
.gpu Spooky
.64bit
.compile_options "-O ccc"
.driver_info "This\nis my\3001 stupid\r\"driver"
.data
    .byte 0x0d, 0x38, 0x42, 0xd5, 0x37, 0x5d, 0x7b, 0x55
    .byte 0xa4, 0xea, 0x15, 0x25, 0x2c, 0xbc
.kernel "kernelxVCR"
    .header
        .byte 0x01, 0x02, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00
        .byte 0x0c, 0x03, 0x06, 0x03, 0x03, 0x02, 0x00, 0x00
        .fill 16, 1, 0x13
        .byte 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x12
        .fill 19, 1, 0x13
    .metadata
        .ascii ";ARGSTART:__OpenCL_DCT_kernel\n"
        .ascii ";version:3:1:111\n"
        .ascii ";device:pitcairn\n"
        .ascii ";uniqueid:1024\n"
        .ascii ";memory:uavprivate:0\n"
        .ascii ";memory:hwlocal:0\n"
        .ascii ";memory:hwregion:0\n"
        .ascii ";pointer:output:float:1:1:0:uav:12:4:RW:0:0\n"
        .ascii ";pointer:input:float:1:1:16:uav:13:4:RO:0:0\n"
        .ascii ";pointer:dct8x8:float:1:1:32:uav:14:4:RO:0:0\n"
        .ascii ";pointer:inter:float:1:1:48:hl:1:4:RW:0:0\n"
        .ascii ";value:width:u32:1:1:64\n"
        .ascii ";value:blockWidth:u32:1:1:80\n"
        .ascii ";value:inverse:u32:1:1:96\n"
        .ascii ";function:1:1030\n"
        .ascii ";uavid:11\n"
        .ascii ";printfid:9\n"
        .ascii ";cbid:10\n"
        .ascii ";privateid:8\n"
        .ascii ";reflection:0:float*\n"
        .ascii ";reflection:1:float*\n"
        .ascii ";reflection:2:float*\n"
        .ascii ";reflection:3:float*\n"
        .ascii ";reflection:4:uint\n"
        .ascii ";reflection:5:uint\n"
        .ascii ";reflection:6:uint\n"
        .ascii ";ARGEND:__OpenCL_DCT_kernel\n"
        .ascii "dessss9843re88888888888888888uuuuuuuufdddessss9843re88888888888888888uuu"
        .ascii "uuuuufdddessss9843re88888888888888888uuuuuuuufdd444444444444444444444444"
        .ascii "\r\r\t\t\t\t\txx\f\f\f\3777x833334441\n"
    .kerneldata
        .byte 0x01, 0x04, 0x05, 0x06, 0x06, 0x06, 0x04, 0x03
        .byte 0x03, 0x05, 0x41, 0x0d, 0x05, 0x05, 0x0b, 0x39
        .byte 0x05, 0x41, 0x05, 0x00, 0x00, 0x00, 0x00, 0x05
        .byte 0x01, 0x0f, 0x05, 0x01, 0x38, 0x00, 0x00, 0x05
        .byte 0x05, 0x41, 0x05, 0x00, 0x00, 0x4e, 0xff, 0x05
    .proginfo
        .entry 0xffcd44dc, 0x00004543
        .entry 0x00000456, 0x05663677
        .entry 0x003cd90c, 0x03958a85
        .entry 0x458c98d9, 0x0344dbd9
        .entry 0x0d0d9d9d, 0x00234455
        .entry 0x00000001, 0x00000055
        .byte 0x65, 0x45, 0x00, 0x00
    .inputs
        .fill 8, 4, 0x00000459
        .int 0x00000459, 0x00000459, 0x00000459, 0x0000045a
        .fill 7, 4, 0x00000459
        .byte 0x59, 0x04, 0x00
    .outputs
        .int 0x00000005, 0x00000006, 0x00000003, 0x0000000d
        .int 0x00000005, 0x00000075, 0x0000000c, 0x00000003
        .int 0x00000311, 0x0000002e, 0x00000037, 0x00000005
        .int 0x00000002, 0x00000003, 0x00000000, 0x00000000
        .byte 0x2c, 0x00, 0x00
    .earlyexit
        .byte 0x4b, 0x05, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00
        .byte 0xbc, 0x01, 0x00, 0x00
    .earlyexit 121
    .floatconsts
        .segment 0, 5
        .segment 66, 57
        .segment 67, 334
        .segment 1, 6
        .segment 5, 86
        .segment 2100, 466
    .inputsamplers
        .sampler 0, 0x5
        .sampler 66, 0x39
        .sampler 67, 0x14e
        .sampler 1, 0x6
        .sampler 5, 0x56
        .sampler 2100, 0x1d2
    .constantbuffers
        .cbmask 0, 5
        .cbmask 66, 57
        .cbmask 67, 334
        .cbmask 1, 6
        .cbmask 5, 86
        .cbmask 2100, 466
    .calnote 0x3d
        .byte 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00
        .byte 0x42, 0x00, 0x00, 0x00, 0x39, 0x00, 0x00, 0x00
        .byte 0x43, 0x00, 0x00, 0x00, 0x4e, 0x01, 0x00, 0x00
        .byte 0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00
        .byte 0x05, 0x00, 0x00, 0x00, 0x56, 0x00, 0x00, 0x00
        .byte 0x34, 0x08, 0x00, 0x00, 0xd2, 0x01, 0x00, 0x00
    .uavopmask 4556
)xxFxx"
    }
};

static void testDisasmData(cxuint testId, const DisasmDataTestCase& testCase)
{
    std::ostringstream disasmOss;
    Disassembler disasm(testCase.amdInput, disasmOss, DISASM_ALL);
    disasm.disassemble();
    std::string resultStr = disasmOss.str();
    if (::strcmp(testCase.expectedString, resultStr.c_str()) != 0)
    {   // print error
        std::ostringstream oss;
        oss << "Failed for #" << testId << std::endl;
        oss.flush();
        throw Exception(oss.str());
    }
}

int main(int argc, const char** argv)
{
    int retVal = 0;
    for (cxuint i = 0; i < sizeof(disasmDataTestCases)/sizeof(DisasmDataTestCase); i++)
        try
        {
            testDisasmData(i, disasmDataTestCases[i]);
        }
        catch(const std::exception& ex)
        {
            std::cerr << ex.what() << std::endl;
            retVal = 1;
        }
    return retVal;
}
