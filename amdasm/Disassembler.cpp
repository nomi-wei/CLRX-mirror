/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014-2016 Mateusz Szpakowski
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
#include <string>
#include <cstring>
#include <ostream>
#include <cstring>
#include <memory>
#include <vector>
#include <utility>
#include <algorithm>
#include <CLRX/utils/Utilities.h>
#include <CLRX/amdbin/AmdBinaries.h>
#include <CLRX/amdbin/GalliumBinaries.h>
#include <CLRX/utils/MemAccess.h>
#include <CLRX/utils/GPUId.h>
#include <CLRX/amdasm/Disassembler.h>

using namespace CLRX;

ISADisassembler::ISADisassembler(Disassembler& _disassembler, cxuint outBufSize)
        : disassembler(_disassembler), output(outBufSize, _disassembler.getOutput())
{ }

ISADisassembler::~ISADisassembler()
{ }

void ISADisassembler::writeLabelsToPosition(size_t pos, LabelIter& labelIter,
              NamedLabelIter& namedLabelIter)
{
    if ((namedLabelIter != namedLabels.end() && namedLabelIter->first <= pos) ||
            (labelIter != labels.end() && *labelIter <= pos))
    {
        size_t curPos = SIZE_MAX;
        if (labelIter != labels.end())
            curPos = *labelIter;
        if (namedLabelIter != namedLabels.end())
            curPos = std::min(curPos, namedLabelIter->first);
        
        bool haveLabel;
        do {
            haveLabel = false;
            const bool haveNumberedLabel =
                    labelIter != labels.end() && *labelIter <= pos;
            const bool haveNamedLabel =
                    (namedLabelIter != namedLabels.end() && namedLabelIter->first <= pos);
            
            size_t namedPos = SIZE_MAX;
            size_t numberedPos = SIZE_MAX;
            if (haveNumberedLabel)
                numberedPos = *labelIter;
            if (haveNamedLabel)
                namedPos = namedLabelIter->first;
            
            /// print numbered (not named) label in form .L[position]_[sectionCount]
            if (numberedPos <= namedPos && haveNumberedLabel)
            {
                curPos = *labelIter;
                char* buf = output.reserve(70);
                size_t bufPos = 0;
                buf[bufPos++] = '.';
                buf[bufPos++] = 'L';
                bufPos += itocstrCStyle(*labelIter, buf+bufPos, 22, 10, 0, false);
                buf[bufPos++] = '_';
                bufPos += itocstrCStyle(disassembler.sectionCount,
                                buf+bufPos, 22, 10, 0, false);
                if (curPos != pos)
                {   // if label shifted back by some bytes before encoded instruction
                    buf[bufPos++] = '=';
                    buf[bufPos++] = '.';
                    buf[bufPos++] = '-';
                    bufPos += itocstrCStyle((pos-curPos), buf+bufPos, 22, 10, 0, false);
                    buf[bufPos++] = '\n';
                }
                else
                {
                    buf[bufPos++] = ':';
                    buf[bufPos++] = '\n';
                }
                output.forward(bufPos);
                ++labelIter;
                haveLabel = true;
            }
            
            /// print named label
            if(namedPos <= numberedPos && haveNamedLabel)
            {
                curPos = namedLabelIter->first;
                output.write(namedLabelIter->second.size(),
                       namedLabelIter->second.c_str());
                char* buf = output.reserve(50);
                size_t bufPos = 0;
                if (curPos != pos)
                {   // if label shifted back by some bytes before encoded instruction
                    buf[bufPos++] = '=';
                    buf[bufPos++] = '.';
                    buf[bufPos++] = '-';
                    bufPos += itocstrCStyle((pos-curPos), buf+bufPos, 22, 10, 0, false);
                    buf[bufPos++] = '\n';
                }
                else
                {
                    buf[bufPos++] = ':';
                    buf[bufPos++] = '\n';
                }
                output.forward(bufPos);
                ++namedLabelIter;
                haveLabel = true;
            }
            
        } while(haveLabel);
    }
}

void ISADisassembler::writeLabelsToEnd(size_t start, LabelIter labelIter,
                   NamedLabelIter namedLabelIter)
{
    size_t pos = start;
    while (namedLabelIter != namedLabels.end() || labelIter != labels.end())
    {
        size_t namedPos = SIZE_MAX;
        size_t numberedPos = SIZE_MAX;
        if (labelIter != labels.end())
            numberedPos = *labelIter;
        if (namedLabelIter != namedLabels.end())
            namedPos = namedLabelIter->first;
        if (numberedPos <= namedPos && labelIter != labels.end())
        {
            if (pos != *labelIter)
            {   // print shift position to label (.org pseudo-op)
                char* buf = output.reserve(30);
                size_t bufPos = 0;
                memcpy(buf+bufPos, ".org ", 5);
                bufPos += 5;
                bufPos += itocstrCStyle(*labelIter, buf+bufPos, 20, 16);
                buf[bufPos++] = '\n';
                output.forward(bufPos);
            }
            char* buf = output.reserve(50);
            size_t bufPos = 0;
            buf[bufPos++] = '.';
            buf[bufPos++] = 'L';
            bufPos += itocstrCStyle(*labelIter, buf+bufPos, 22, 10, 0, false);
            buf[bufPos++] = '_';
            bufPos += itocstrCStyle(disassembler.sectionCount,
                            buf+bufPos, 22, 10, 0, false);
            buf[bufPos++] = ':';
            buf[bufPos++] = '\n';
            output.forward(bufPos);
            pos = *labelIter;
            ++labelIter;
        }
        if (namedPos <= numberedPos && namedLabelIter != namedLabels.end())
        {
            if (pos != namedLabelIter->first)
            {   // print shift position to label (.org pseudo-op)
                char* buf = output.reserve(30);
                size_t bufPos = 0;
                memcpy(buf+bufPos, ".org ", 5);
                bufPos += 5;
                bufPos += itocstrCStyle(namedLabelIter->first, buf+bufPos, 20, 16);
                buf[bufPos++] = '\n';
                output.forward(bufPos);
            }
            output.write(namedLabelIter->second.size(),
                        namedLabelIter->second.c_str());
            pos = namedLabelIter->first;
            ++namedLabelIter;
        }
    }
}

void ISADisassembler::writeLocation(size_t pos)
{
    const auto namedLabelIt = binaryMapFind(namedLabels.begin(), namedLabels.end(), pos);
    if (namedLabelIt != namedLabels.end())
    {   /* print named label */
        output.write(namedLabelIt->second.size(), namedLabelIt->second.c_str());
        return;
    }
    /* otherwise we print numbered label */
    char* buf = output.reserve(50);
    size_t bufPos = 0;
    buf[bufPos++] = '.';
    buf[bufPos++] = 'L';
    bufPos += itocstrCStyle(pos, buf+bufPos, 22, 10, 0, false);
    buf[bufPos++] = '_';
    bufPos += itocstrCStyle(disassembler.sectionCount, buf+bufPos, 22, 10, 0, false);
    output.forward(bufPos);
}

bool ISADisassembler::writeRelocation(size_t pos, RelocIter& relocIter)
{
    while (relocIter != relocations.end() && relocIter->first < pos)
        relocIter++;
    if (relocIter == relocations.end() || relocIter->first != pos)
        return false;
    const Relocation& reloc = relocIter->second;
    if (reloc.addend != 0 && 
        (reloc.type==RELTYPE_LOW_32BIT || reloc.type==RELTYPE_HIGH_32BIT))
        output.write(1, "(");
    /// write name+value
    output.writeString(relSymbols[reloc.symbol].c_str());
    char* buf = output.reserve(50);
    size_t bufPos = 0;
    if (reloc.addend != 0)
    {
        if (reloc.addend > 0)
            buf[bufPos++] = '+';
        bufPos += itocstrCStyle(reloc.addend, buf+bufPos, 22, 10, 0, false);
        if (reloc.type==RELTYPE_LOW_32BIT || reloc.type==RELTYPE_HIGH_32BIT)
            buf[bufPos++] = ')';
    }
    if (reloc.type==RELTYPE_LOW_32BIT)
    {
        ::memcpy(buf+bufPos, "&0xffffffff", 11);
        bufPos += 11;
    }
    else if (reloc.type==RELTYPE_HIGH_32BIT)
    {
        ::memcpy(buf+bufPos, ">>32", 4);
        bufPos += 4;
    }
    output.forward(bufPos);
    ++relocIter;
    return true;
}

/* helpers for main Disassembler class */

struct CLRX_INTERNAL CL2GPUDeviceCodeEntry
{
    uint32_t elfFlags;
    GPUDeviceType deviceType;
};

static const CL2GPUDeviceCodeEntry cl2GpuDeviceCodeTable[] =
{
    { 6, GPUDeviceType::BONAIRE },
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 4, GPUDeviceType::MULLINS },
    { 17, GPUDeviceType::FIJI },
    { 16, GPUDeviceType::CARRIZO },
    { 15, GPUDeviceType::DUMMY }
};

static const CL2GPUDeviceCodeEntry cl2_16_3GpuDeviceCodeTable[] =
{
    { 6, GPUDeviceType::BONAIRE },
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 4, GPUDeviceType::MULLINS },
    { 16, GPUDeviceType::FIJI },
    { 15, GPUDeviceType::CARRIZO },
    { 13, GPUDeviceType::GOOSE },
    { 12, GPUDeviceType::HORSE },
    { 17, GPUDeviceType::STONEY }
};

static const CL2GPUDeviceCodeEntry cl2_15_7GpuDeviceCodeTable[] =
{
    { 6, GPUDeviceType::BONAIRE },
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 4, GPUDeviceType::MULLINS },
    { 16, GPUDeviceType::FIJI },
    { 15, GPUDeviceType::CARRIZO }
};
/* driver version: 2036.3 */
static const CL2GPUDeviceCodeEntry cl2GPUPROGpuDeviceCodeTable[] =
{
    { 6, GPUDeviceType::BONAIRE },
    { 1, GPUDeviceType::SPECTRE },
    { 2, GPUDeviceType::SPOOKY },
    { 3, GPUDeviceType::KALINDI },
    { 7, GPUDeviceType::HAWAII },
    { 8, GPUDeviceType::ICELAND },
    { 9, GPUDeviceType::TONGA },
    { 4, GPUDeviceType::MULLINS },
    { 14, GPUDeviceType::FIJI },
    { 13, GPUDeviceType::CARRIZO },
    { 17, GPUDeviceType::ELLESMERE },
    { 16, GPUDeviceType::BAFFIN },
    { 15, GPUDeviceType::STONEY }
};

struct CLRX_INTERNAL GPUDeviceCodeEntry
{
    uint16_t elfMachine;
    GPUDeviceType deviceType;
};

static const GPUDeviceCodeEntry gpuDeviceCodeTable[16] =
{
    { 0x3fd, GPUDeviceType::TAHITI },
    { 0x3fe, GPUDeviceType::PITCAIRN },
    { 0x3ff, GPUDeviceType::CAPE_VERDE },
    { 0x402, GPUDeviceType::OLAND },
    { 0x403, GPUDeviceType::BONAIRE },
    { 0x404, GPUDeviceType::SPECTRE },
    { 0x405, GPUDeviceType::SPOOKY },
    { 0x406, GPUDeviceType::KALINDI },
    { 0x407, GPUDeviceType::HAINAN },
    { 0x408, GPUDeviceType::HAWAII },
    { 0x409, GPUDeviceType::ICELAND },
    { 0x40a, GPUDeviceType::TONGA },
    { 0x40b, GPUDeviceType::MULLINS },
    { 0x40c, GPUDeviceType::FIJI },
    { 0x40d, GPUDeviceType::CARRIZO },
    { 0x411, GPUDeviceType::DUMMY }
};

struct CLRX_INTERNAL GPUDeviceInnerCodeEntry
{
    uint32_t dMachine;
    GPUDeviceType deviceType;
};

static const GPUDeviceInnerCodeEntry gpuDeviceInnerCodeTable[16] =
{
    { 0x1a, GPUDeviceType::TAHITI },
    { 0x1b, GPUDeviceType::PITCAIRN },
    { 0x1c, GPUDeviceType::CAPE_VERDE },
    { 0x20, GPUDeviceType::OLAND },
    { 0x21, GPUDeviceType::BONAIRE },
    { 0x22, GPUDeviceType::SPECTRE },
    { 0x23, GPUDeviceType::SPOOKY },
    { 0x24, GPUDeviceType::KALINDI },
    { 0x25, GPUDeviceType::HAINAN },
    { 0x27, GPUDeviceType::HAWAII },
    { 0x29, GPUDeviceType::ICELAND },
    { 0x2a, GPUDeviceType::TONGA },
    { 0x2b, GPUDeviceType::MULLINS },
    { 0x2d, GPUDeviceType::FIJI },
    { 0x2e, GPUDeviceType::CARRIZO },
    { 0x31, GPUDeviceType::DUMMY }
};

static void getAmdDisasmKernelInputFromBinary(const AmdInnerGPUBinary32* innerBin,
        AmdDisasmKernelInput& kernelInput, Flags flags, GPUDeviceType inputDeviceType)
{
    const cxuint entriesNum = sizeof(gpuDeviceCodeTable)/sizeof(GPUDeviceCodeEntry);
    kernelInput.codeSize = kernelInput.dataSize = 0;
    kernelInput.code = kernelInput.data = nullptr;
    
    if (innerBin != nullptr)
    {   // if innerBinary exists
        bool codeFound = false;
        bool dataFound = false;
        cxuint encEntryIndex = 0;
        for (encEntryIndex = 0; encEntryIndex < innerBin->getCALEncodingEntriesNum();
             encEntryIndex++)
        {
            const CALEncodingEntry& encEntry =
                    innerBin->getCALEncodingEntry(encEntryIndex);
            /* check gpuDeviceType */
            const uint32_t dMachine = ULEV(encEntry.machine);
            cxuint index;
            // detect GPU device from machine field from CAL encoding entry
            for (index = 0; index < entriesNum; index++)
                if (gpuDeviceInnerCodeTable[index].dMachine == dMachine)
                    break;
            if (entriesNum != index &&
                gpuDeviceInnerCodeTable[index].deviceType == inputDeviceType)
                break; // if found
        }
        if (encEntryIndex == innerBin->getCALEncodingEntriesNum())
            throw Exception("Can't find suitable CALEncodingEntry!");
        const CALEncodingEntry& encEntry =
                    innerBin->getCALEncodingEntry(encEntryIndex);
        const size_t encEntryOffset = ULEV(encEntry.offset);
        const size_t encEntrySize = ULEV(encEntry.size);
        /* find suitable sections */
        for (cxuint j = 0; j < innerBin->getSectionHeadersNum(); j++)
        {
            const char* secName = innerBin->getSectionName(j);
            const Elf32_Shdr& shdr = innerBin->getSectionHeader(j);
            const size_t secOffset = ULEV(shdr.sh_offset);
            const size_t secSize = ULEV(shdr.sh_size);
            if (secOffset < encEntryOffset ||
                    usumGt(secOffset, secSize, encEntryOffset+encEntrySize))
                continue; // skip this section (not in choosen encoding)
            
            if (!codeFound && ::strcmp(secName, ".text") == 0)
            {
                kernelInput.codeSize = secSize;
                kernelInput.code = innerBin->getSectionContent(j);
                codeFound = true;
            }
            else if (!dataFound && ::strcmp(secName, ".data") == 0)
            {
                kernelInput.dataSize = secSize;
                kernelInput.data = innerBin->getSectionContent(j);
                dataFound = true;
            }
            
            if (codeFound && dataFound)
                break; // end of finding
        }
        
        if ((flags & DISASM_CALNOTES) != 0)
        {
            kernelInput.calNotes.resize(innerBin->getCALNotesNum(encEntryIndex));
            cxuint j = 0;
            for (const CALNote& calNote: innerBin->getCALNotes(encEntryIndex))
            {
                CALNoteInput& outCalNote = kernelInput.calNotes[j++];
                outCalNote.header.nameSize = ULEV(calNote.header->nameSize);
                outCalNote.header.type = ULEV(calNote.header->type);
                outCalNote.header.descSize = ULEV(calNote.header->descSize);
                std::copy(calNote.header->name, calNote.header->name+8,
                          outCalNote.header.name);
                outCalNote.data = calNote.data;
            }
        }
    }
}

static AmdCL2DisasmInput* getAmdCL2DisasmInputFromBinary(const AmdCL2MainGPUBinary& binary)
{
    std::unique_ptr<AmdCL2DisasmInput> input(new AmdCL2DisasmInput);
    const uint32_t elfFlags = ULEV(binary.getHeader().e_flags);
    // detect GPU device from elfMachine field from ELF header
    cxuint entriesNum = 0;
    const CL2GPUDeviceCodeEntry* gpuCodeTable = nullptr;
    input->driverVersion = binary.getDriverVersion();
    if (input->driverVersion < 191205)
    {
        gpuCodeTable = cl2_15_7GpuDeviceCodeTable;
        entriesNum = sizeof(cl2_15_7GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry);
    }
    else if (input->driverVersion < 200406)
    {
        gpuCodeTable = cl2GpuDeviceCodeTable;
        entriesNum = sizeof(cl2GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry);
    }
    else if (input->driverVersion < 203603)
    {
        gpuCodeTable = cl2_16_3GpuDeviceCodeTable;
        entriesNum = sizeof(cl2_16_3GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry);
    }
    else
    {
        gpuCodeTable = cl2GPUPROGpuDeviceCodeTable;
        entriesNum = sizeof(cl2GPUPROGpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry);
    }
    //const cxuint entriesNum = sizeof(cl2GpuDeviceCodeTable)/sizeof(CL2GPUDeviceCodeEntry);
    cxuint index;
    for (index = 0; index < entriesNum; index++)
        if (gpuCodeTable[index].elfFlags == elfFlags)
            break;
    if (entriesNum == index)
        throw Exception("Can't determine GPU device type");
    
    input->deviceType = gpuCodeTable[index].deviceType;
    input->compileOptions = binary.getCompileOptions();
    input->aclVersionString = binary.getAclVersionString();
    bool isInnerNewBinary = binary.hasInnerBinary() &&
                binary.getDriverVersion()>=191205;
    
    input->samplerInitSize = 0;
    input->samplerInit = nullptr;
    input->globalDataSize = 0;
    input->globalData = nullptr;
    input->rwDataSize = 0;
    input->rwData = nullptr;
    input->bssAlignment = input->bssSize = 0;
    std::vector<std::pair<size_t, size_t> > sortedRelocs; // by offset
    const cxbyte* textPtr = nullptr;
    const size_t kernelInfosNum = binary.getKernelInfosNum();
    
    uint16_t gDataSectionIdx = SHN_UNDEF;
    uint16_t rwDataSectionIdx = SHN_UNDEF;
    uint16_t bssDataSectionIdx = SHN_UNDEF;
    
    if (isInnerNewBinary)
    {
        const AmdCL2InnerGPUBinary& innerBin = binary.getInnerBinary();
        input->globalDataSize = innerBin.getGlobalDataSize();
        input->globalData = innerBin.getGlobalData();
        input->rwDataSize = innerBin.getRwDataSize();
        input->rwData = innerBin.getRwData();
        input->samplerInitSize = innerBin.getSamplerInitSize();
        input->samplerInit = innerBin.getSamplerInit();
        input->bssAlignment = innerBin.getBssAlignment();
        input->bssSize = innerBin.getBssSize();
        // if no kernels and data
        if (kernelInfosNum==0)
            return input.release();
        
        size_t relaNum = innerBin.getTextRelaEntriesNum();
        for (size_t i = 0; i < relaNum; i++)
        {
            const Elf64_Rela& rel = innerBin.getTextRelaEntry(i);
            sortedRelocs.push_back(std::make_pair(size_t(ULEV(rel.r_offset)), i));
        }
        // sort map
        mapSort(sortedRelocs.begin(), sortedRelocs.end());
        // get code section pointer for determine relocation position kernel code
        textPtr = innerBin.getSectionContent(".hsatext");
        
        try
        { gDataSectionIdx = innerBin.getSectionIndex(".hsadata_readonly_agent"); }
        catch(const Exception& ex)
        { }
        try
        { rwDataSectionIdx = innerBin.getSectionIndex(".hsadata_global_agent"); }
        catch(const Exception& ex)
        { }
        try
        { bssDataSectionIdx = innerBin.getSectionIndex(".hsabss_global_agent"); }
        catch(const Exception& ex)
        { }
        // relocations for global data section (sampler symbols)
        relaNum = innerBin.getGlobalDataRelaEntriesNum();
        // section index for samplerinit (will be used for comparing sampler symbol section
        uint16_t samplerInitSecIndex = SHN_UNDEF;
        try
        { samplerInitSecIndex = innerBin.getSectionIndex(".hsaimage_samplerinit"); }
        catch(const Exception& ex)
        { }
        
        for (size_t i = 0; i < relaNum; i++)
        {
            const Elf64_Rela& rel = innerBin.getGlobalDataRelaEntry(i);
            size_t symIndex = ELF64_R_SYM(ULEV(rel.r_info));
            const Elf64_Sym& sym = innerBin.getSymbol(symIndex);
            // check symbol type, section and value
            if (ELF64_ST_TYPE(sym.st_info) != 12)
                throw Exception("Wrong sampler symbol");
            uint64_t value = ULEV(sym.st_value);
            if (ULEV(sym.st_shndx) != samplerInitSecIndex)
                throw Exception("Wrong section for sampler symbol");
            if ((value&7) != 0)
                throw Exception("Wrong value of sampler symbol");
            input->samplerRelocs.push_back({ size_t(ULEV(rel.r_offset)),
                size_t(value>>3) });
        }
    }
    else if (kernelInfosNum==0)
        return input.release();
    
    input->kernels.resize(kernelInfosNum);
    auto sortedRelocIter = sortedRelocs.begin();
    
    for (cxuint i = 0; i < kernelInfosNum; i++)
    {
        const KernelInfo& kernelInfo = binary.getKernelInfo(i);
        AmdCL2DisasmKernelInput& kinput = input->kernels[i];
        kinput.kernelName = kernelInfo.kernelName;
        kinput.metadataSize = binary.getMetadataSize(i);
        kinput.metadata = binary.getMetadata(i);
        
        kinput.isaMetadataSize = 0;
        kinput.isaMetadata = nullptr;
        // setup isa metadata content
        const AmdCL2GPUKernelMetadata* isaMetadata = nullptr;
        if (i < binary.getISAMetadatasNum())
            isaMetadata = &binary.getISAMetadataEntry(i);
        if (isaMetadata == nullptr || isaMetadata->kernelName != kernelInfo.kernelName)
        {   // fallback if not in order
            try
            { isaMetadata = &binary.getISAMetadataEntry(
                            kernelInfo.kernelName.c_str()); }
            catch(const Exception& ex) // failed
            { isaMetadata = nullptr; }
        }
        if (isaMetadata!=nullptr)
        {
            kinput.isaMetadataSize = isaMetadata->size;
            kinput.isaMetadata = isaMetadata->data;
        }
        
        kinput.code = nullptr;
        kinput.codeSize = 0;
        kinput.setup = nullptr;
        kinput.setupSize = 0;
        kinput.stub = nullptr;
        kinput.stubSize = 0;
        if (!binary.hasInnerBinary())
            continue; // nothing else to set
        
        // get kernel code, setup and stub content
        const AmdCL2InnerGPUBinaryBase& innerBin = binary.getInnerBinaryBase();
        const AmdCL2GPUKernel* kernelData = nullptr;
        if (i < innerBin.getKernelsNum())
            kernelData = &innerBin.getKernelData(i);
        if (kernelData==nullptr || kernelData->kernelName != kernelInfo.kernelName)
            kernelData = &innerBin.getKernelData(kernelInfo.kernelName.c_str());
        
        if (kernelData!=nullptr)
        {
            kinput.code = kernelData->code;
            kinput.codeSize = kernelData->codeSize;
            kinput.setup = kernelData->setup;
            kinput.setupSize = kernelData->setupSize;
        }
        if (!isInnerNewBinary)
        {   // old drivers
            const AmdCL2OldInnerGPUBinary& oldInnerBin = binary.getOldInnerBinary();
            const AmdCL2GPUKernelStub* kstub = nullptr;
            if (i < innerBin.getKernelsNum())
                kstub = &oldInnerBin.getKernelStub(i);
            if (kstub==nullptr || kernelData->kernelName != kernelInfo.kernelName)
                kstub = &oldInnerBin.getKernelStub(kernelInfo.kernelName.c_str());
            if (kstub!=nullptr)
            {
                kinput.stubSize = kstub->size;
                kinput.stub = kstub->data;
            }
        }
        else
        {   // relocations
            const AmdCL2InnerGPUBinary& innerBin = binary.getInnerBinary();
            
            if (sortedRelocIter != sortedRelocs.end() &&
                    sortedRelocIter->first < size_t(kinput.code-textPtr))
                throw Exception("Code relocation offset outside kernel code");
            
            if (sortedRelocIter != sortedRelocs.end())
            {
                size_t end = kinput.code+kinput.codeSize-textPtr;
                for (; sortedRelocIter != sortedRelocs.end() &&
                    sortedRelocIter->first<=end; ++sortedRelocIter)
                {   // add relocations
                    const Elf64_Rela& rela = innerBin.getTextRelaEntry(
                                sortedRelocIter->second);
                    uint32_t symIndex = ELF64_R_SYM(ULEV(rela.r_info));
                    int64_t addend = ULEV(rela.r_addend);
                    cxuint rsym = 0;
                    // check this symbol
                    const Elf64_Sym& sym = innerBin.getSymbol(symIndex);
                    uint16_t symShndx = ULEV(sym.st_shndx);
                    if (symShndx!=gDataSectionIdx && symShndx!=rwDataSectionIdx &&
                        symShndx!=bssDataSectionIdx)
                        throw Exception("Symbol is not placed in global or "
                                "rwdata data or bss is illegal");
                    addend += ULEV(sym.st_value);
                    rsym = (symShndx==rwDataSectionIdx) ? 1 : 
                        ((symShndx==bssDataSectionIdx) ? 2 : 0);
                    
                    RelocType relocType;
                    uint32_t rtype = ELF64_R_TYPE(ULEV(rela.r_info));
                    if (rtype==1)
                        relocType = RELTYPE_LOW_32BIT;
                    else if (rtype==2)
                        relocType = RELTYPE_HIGH_32BIT;
                    else
                        throw Exception("Unknown relocation type");
                    // put text relocs. compute offset by subtracting current code offset
                    kinput.textRelocs.push_back(AmdCL2RelaEntry{sortedRelocIter->first-
                        (kinput.code-textPtr), relocType, rsym, addend });
                }
            }
        }
    }
    if (sortedRelocIter != sortedRelocs.end())
        throw Exception("Code relocation offset outside kernel code");
    return input.release();
}

template<typename AmdMainBinary>
static AmdDisasmInput* getAmdDisasmInputFromBinary(const AmdMainBinary& binary,
           Flags flags)
{
    std::unique_ptr<AmdDisasmInput> input(new AmdDisasmInput);
    cxuint index = 0;
    const uint16_t elfMachine = ULEV(binary.getHeader().e_machine);
    input->is64BitMode = (binary.getHeader().e_ident[EI_CLASS] == ELFCLASS64);
    const cxuint entriesNum = sizeof(gpuDeviceCodeTable)/sizeof(GPUDeviceCodeEntry);
    // detect GPU device from elfMachine field from ELF header
    for (index = 0; index < entriesNum; index++)
        if (gpuDeviceCodeTable[index].elfMachine == elfMachine)
            break;
    if (entriesNum == index)
        throw Exception("Can't determine GPU device type");
    
    input->deviceType = gpuDeviceCodeTable[index].deviceType;
    input->compileOptions = binary.getCompileOptions();
    input->driverInfo = binary.getDriverInfo();
    input->globalDataSize = binary.getGlobalDataSize();
    input->globalData = binary.getGlobalData();
    const size_t kernelInfosNum = binary.getKernelInfosNum();
    const size_t kernelHeadersNum = binary.getKernelHeadersNum();
    const size_t innerBinariesNum = binary.getInnerBinariesNum();
    input->kernels.resize(kernelInfosNum);
    
    for (cxuint i = 0; i < kernelInfosNum; i++)
    {
        const KernelInfo& kernelInfo = binary.getKernelInfo(i);
        const AmdInnerGPUBinary32* innerBin = nullptr;
        if (i < innerBinariesNum)
            innerBin = &binary.getInnerBinary(i);
        if (innerBin == nullptr || innerBin->getKernelName() != kernelInfo.kernelName)
        {   // fallback if not in order
            try
            { innerBin = &binary.getInnerBinary(kernelInfo.kernelName.c_str()); }
            catch(const Exception& ex)
            { innerBin = nullptr; }
        }
        AmdDisasmKernelInput& kernelInput = input->kernels[i];
        kernelInput.metadataSize = binary.getMetadataSize(i);
        kernelInput.metadata = binary.getMetadata(i);
        
        // kernel header
        kernelInput.headerSize = 0;
        kernelInput.header = nullptr;
        const AmdGPUKernelHeader* khdr = nullptr;
        if (i < kernelHeadersNum)
            khdr = &binary.getKernelHeaderEntry(i);
        if (khdr == nullptr || khdr->kernelName != kernelInfo.kernelName)
        {   // fallback if not in order
            try
            { khdr = &binary.getKernelHeaderEntry(kernelInfo.kernelName.c_str()); }
            catch(const Exception& ex) // failed
            { khdr = nullptr; }
        }
        if (khdr != nullptr)
        {
            kernelInput.headerSize = khdr->size;
            kernelInput.header = khdr->data;
        }
        
        kernelInput.kernelName = kernelInfo.kernelName;
        getAmdDisasmKernelInputFromBinary(innerBin, kernelInput, flags, input->deviceType);
    }
    return input.release();
}

template<typename GalliumElfBinary>
static void getGalliumDisasmInputFromBinaryBase(const GalliumBinary& binary,
            const GalliumElfBinary& elfBin, Flags flags, GalliumDisasmInput* input)
{
    uint16_t rodataIndex = SHN_UNDEF;
    try
    { rodataIndex = elfBin.getSectionIndex(".rodata"); }
    catch(const Exception& ex)
    { }
    const uint16_t textIndex = elfBin.getSectionIndex(".text");
    
    if (rodataIndex != SHN_UNDEF)
    {
        input->globalData = elfBin.getSectionContent(rodataIndex);
        input->globalDataSize = ULEV(elfBin.getSectionHeader(rodataIndex).sh_size);
    }
    else
    {
        input->globalDataSize = 0;
        input->globalData = nullptr;
    }
    // kernels
    input->kernels.resize(binary.getKernelsNum());
    for (cxuint i = 0; i < binary.getKernelsNum(); i++)
    {
        const GalliumKernel& kernel = binary.getKernel(i);
        const GalliumProgInfoEntry* progInfo = elfBin.getProgramInfo(i);
        GalliumProgInfoEntry outProgInfo[3];
        for (cxuint k = 0; k < 3; k++)
        {
            outProgInfo[k].address = ULEV(progInfo[k].address);
            outProgInfo[k].value = ULEV(progInfo[k].value);
        }
        input->kernels[i] = { kernel.kernelName,
            {outProgInfo[0],outProgInfo[1],outProgInfo[2]}, kernel.offset,
            std::vector<GalliumArgInfo>(kernel.argInfos.begin(), kernel.argInfos.end()) };
    }
    input->code = elfBin.getSectionContent(textIndex);
    input->codeSize = ULEV(elfBin.getSectionHeader(textIndex).sh_size);
}

static GalliumDisasmInput* getGalliumDisasmInputFromBinary(GPUDeviceType deviceType,
           const GalliumBinary& binary, Flags flags)
{
    std::unique_ptr<GalliumDisasmInput> input(new GalliumDisasmInput);
    input->deviceType = deviceType;
    if (!binary.is64BitElfBinary())
    {
        input->is64BitMode = false;
        getGalliumDisasmInputFromBinaryBase(binary, binary.getElfBinary32(),
                                flags, input.get());
    }
    else // 64-bit
    {
        input->is64BitMode = true;
        getGalliumDisasmInputFromBinaryBase(binary, binary.getElfBinary64(),
                                flags, input.get());
    }
    return input.release();
}

Disassembler::Disassembler(const AmdMainGPUBinary32& binary, std::ostream& _output,
            Flags _flags) : fromBinary(true), binaryFormat(BinaryFormat::AMD),
            amdInput(nullptr), output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
    amdInput = getAmdDisasmInputFromBinary(binary, flags);
}

Disassembler::Disassembler(const AmdMainGPUBinary64& binary, std::ostream& _output,
            Flags _flags) : fromBinary(true), binaryFormat(BinaryFormat::AMD),
            amdInput(nullptr), output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
    amdInput = getAmdDisasmInputFromBinary(binary, flags);
}

Disassembler::Disassembler(const AmdCL2MainGPUBinary& binary, std::ostream& _output,
           Flags _flags) : fromBinary(true), binaryFormat(BinaryFormat::AMDCL2),
            amdCL2Input(nullptr), output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
    amdCL2Input = getAmdCL2DisasmInputFromBinary(binary);
}

Disassembler::Disassembler(const AmdDisasmInput* disasmInput, std::ostream& _output,
            Flags _flags) : fromBinary(false), binaryFormat(BinaryFormat::AMD),
            amdInput(disasmInput), output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
}

Disassembler::Disassembler(const AmdCL2DisasmInput* disasmInput, std::ostream& _output,
            Flags _flags) : fromBinary(false), binaryFormat(BinaryFormat::AMDCL2),
            amdCL2Input(disasmInput), output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
}

Disassembler::Disassembler(GPUDeviceType deviceType, const GalliumBinary& binary,
           std::ostream& _output, Flags _flags) :
           fromBinary(true), binaryFormat(BinaryFormat::GALLIUM),
           galliumInput(nullptr), output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
    galliumInput = getGalliumDisasmInputFromBinary(deviceType, binary, flags);
}

Disassembler::Disassembler(const GalliumDisasmInput* disasmInput, std::ostream& _output,
             Flags _flags) : fromBinary(false), binaryFormat(BinaryFormat::GALLIUM),
            galliumInput(disasmInput), output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
}

Disassembler::Disassembler(GPUDeviceType deviceType, size_t rawCodeSize,
           const cxbyte* rawCode, std::ostream& _output, Flags _flags)
       : fromBinary(true), binaryFormat(BinaryFormat::RAWCODE),
         output(_output), flags(_flags), sectionCount(0)
{
    isaDisassembler.reset(new GCNDisassembler(*this));
    rawInput = new RawCodeInput{ deviceType, rawCodeSize, rawCode };
}

Disassembler::~Disassembler()
{
    if (fromBinary)
    {
        if (binaryFormat == BinaryFormat::AMD)
            delete amdInput;
        else if (binaryFormat == BinaryFormat::GALLIUM)
            delete galliumInput;
        else if (binaryFormat == BinaryFormat::AMDCL2)
            delete amdCL2Input;
        else // raw code input
            delete rawInput;
    }
}

GPUDeviceType Disassembler::getDeviceType() const
{
    if (binaryFormat == BinaryFormat::AMD)
        return amdInput->deviceType;
    else if (binaryFormat == BinaryFormat::AMDCL2)
        return amdCL2Input->deviceType;
    else if (binaryFormat == BinaryFormat::GALLIUM)
        return galliumInput->deviceType;
    else // rawcode
        return rawInput->deviceType;
}

static void printDisasmData(size_t size, const cxbyte* data, std::ostream& output,
                bool secondAlign = false)
{
    char buf[68];
    /// const strings for .byte and fill pseudo-ops
    const char* linePrefix = "    .byte ";
    const char* fillPrefix = "    .fill ";
    size_t prefixSize = 10;
    if (secondAlign)
    {   // const string for double alignment
        linePrefix = "        .byte ";
        fillPrefix = "        .fill ";
        prefixSize += 4;
    }
    ::memcpy(buf, linePrefix, prefixSize);
    for (size_t p = 0; p < size;)
    {
        size_t fillEnd;
        // find max repetition of this element
        for (fillEnd = p+1; fillEnd < size && data[fillEnd]==data[p]; fillEnd++);
        if (fillEnd >= p+8)
        {   // if element repeated for least 1 line
            // print .fill pseudo-op
            ::memcpy(buf, fillPrefix, prefixSize);
            const size_t oldP = p;
            p = (fillEnd != size) ? fillEnd&~size_t(7) : fillEnd;
            size_t bufPos = prefixSize;
            bufPos += itocstrCStyle(p-oldP, buf+bufPos, 22, 10);
            memcpy(buf+bufPos, ", 1, ", 5);
            bufPos += 5;
            bufPos += itocstrCStyle(data[oldP], buf+bufPos, 6, 16, 2);
            buf[bufPos++] = '\n';
            output.write(buf, bufPos);
            ::memcpy(buf, linePrefix, prefixSize);
            continue;
        }
        
        const size_t lineEnd = std::min(p+8, size);
        size_t bufPos = prefixSize;
        // print 8 or less (if end of data) bytes
        for (; p < lineEnd; p++)
        {
            buf[bufPos++] = '0';
            buf[bufPos++] = 'x';
            {   // inline byte in hexadecimal
                cxuint digit = data[p]>>4;
                if (digit < 10)
                    buf[bufPos++] = '0'+digit;
                else
                    buf[bufPos++] = 'a'+digit-10;
                digit = data[p]&0xf;
                if (digit < 10)
                    buf[bufPos++] = '0'+digit;
                else
                    buf[bufPos++] = 'a'+digit-10;
            }
            if (p+1 < lineEnd)
            {
                buf[bufPos++] = ',';
                buf[bufPos++] = ' ';
            }
        }
        buf[bufPos++] = '\n';
        output.write(buf, bufPos);
    }
}

static void printDisasmDataU32(size_t size, const uint32_t* data, std::ostream& output,
                bool secondAlign = false)
{
    char buf[68];
    /// const strings for .byte and fill pseudo-ops
    const char* linePrefix = "    .int ";
    const char* fillPrefix = "    .fill ";
    size_t fillPrefixSize = 10;
    if (secondAlign)
    {   // const string for double alignment
        linePrefix = "        .int ";
        fillPrefix = "        .fill ";
        fillPrefixSize += 4;
    }
    const size_t intPrefixSize = fillPrefixSize-1;
    ::memcpy(buf, linePrefix, intPrefixSize);
    for (size_t p = 0; p < size;)
    {
        size_t fillEnd;
        // find max repetition of this char
        for (fillEnd = p+1; fillEnd < size && ULEV(data[fillEnd])==ULEV(data[p]);
             fillEnd++);
        if (fillEnd >= p+4)
        {   // if element repeated for least 1 line
            // print .fill pseudo-op
            ::memcpy(buf, fillPrefix, fillPrefixSize);
            const size_t oldP = p;
            p = (fillEnd != size) ? fillEnd&~size_t(3) : fillEnd;
            size_t bufPos = fillPrefixSize;
            bufPos += itocstrCStyle(p-oldP, buf+bufPos, 22, 10);
            memcpy(buf+bufPos, ", 4, ", 5);
            bufPos += 5;
            bufPos += itocstrCStyle(ULEV(data[oldP]), buf+bufPos, 12, 16, 8);
            buf[bufPos++] = '\n';
            output.write(buf, bufPos);
            ::memcpy(buf, linePrefix, fillPrefixSize);
            continue;
        }
        
        const size_t lineEnd = std::min(p+4, size);
        size_t bufPos = intPrefixSize;
        // print four or less (if end of data) dwords
        for (; p < lineEnd; p++)
        {
            bufPos += itocstrCStyle(ULEV(data[p]), buf+bufPos, 12, 16, 8);
            if (p+1 < lineEnd)
            {
                buf[bufPos++] = ',';
                buf[bufPos++] = ' ';
            }
        }
        buf[bufPos++] = '\n';
        output.write(buf, bufPos);
    }
}

static void printDisasmLongString(size_t size, const char* data, std::ostream& output,
            bool secondAlign = false)
{
    
    const char* linePrefix = "    .ascii \"";
    size_t prefixSize = 12;
    if (secondAlign)
    {
        linePrefix = "        .ascii \"";
        prefixSize += 4;
    }
    char buffer[96];
    ::memcpy(buffer, linePrefix, prefixSize);
    
    for (size_t pos = 0; pos < size; )
    {
        const size_t end = std::min(pos+72, size);
        const size_t oldPos = pos;
        while (pos < end && data[pos] != '\n') pos++;
        if (pos < end && data[pos] == '\n') pos++; // embrace newline
        size_t escapeSize;
        pos = oldPos + escapeStringCStyle(pos-oldPos, data+oldPos, 76,
                      buffer+prefixSize, escapeSize);
        buffer[prefixSize+escapeSize] = '\"';
        buffer[prefixSize+escapeSize+1] = '\n';
        output.write(buffer, prefixSize+escapeSize+2);
    }
}

static const char* disasmCALNoteNamesTable[] =
{
    ".proginfo",
    ".inputs",
    ".outputs",
    ".condout",
    ".floatconsts",
    ".intconsts",
    ".boolconsts",
    ".earlyexit",
    ".globalbuffers",
    ".constantbuffers",
    ".inputsamplers",
    ".persistentbuffers",
    ".scratchbuffers",
    ".subconstantbuffers",
    ".uavmailboxsize",
    ".uav",
    ".uavopmask"
};

void Disassembler::disassembleAmd()
{
    if (amdInput->is64BitMode)
        output.write(".64bit\n", 7);
    else
        output.write(".32bit\n", 7);
    
    const bool doMetadata = ((flags & DISASM_METADATA) != 0);
    const bool doDumpData = ((flags & DISASM_DUMPDATA) != 0);
    const bool doDumpCode = ((flags & DISASM_DUMPCODE) != 0);
    
    if (doMetadata)
    {
        output.write(".compile_options \"", 18);
        const std::string escapedCompileOptions = 
                escapeStringCStyle(amdInput->compileOptions);
        output.write(escapedCompileOptions.c_str(), escapedCompileOptions.size());
        output.write("\"\n.driver_info \"", 16);
        const std::string escapedDriverInfo =
                escapeStringCStyle(amdInput->driverInfo);
        output.write(escapedDriverInfo.c_str(), escapedDriverInfo.size());
        output.write("\"\n", 2);
    }
    
    if (doDumpData && amdInput->globalData != nullptr && amdInput->globalDataSize != 0)
    {   //
        output.write(".globaldata\n", 12);
        printDisasmData(amdInput->globalDataSize, amdInput->globalData, output);
    }
    
    for (const AmdDisasmKernelInput& kinput: amdInput->kernels)
    {
        output.write(".kernel ", 8);
        output.write(kinput.kernelName.c_str(), kinput.kernelName.size());
        output.put('\n');
        if (doMetadata)
        {
            if (kinput.header != nullptr && kinput.headerSize != 0)
            {   // if kernel header available
                output.write("    .header\n", 12);
                printDisasmData(kinput.headerSize, kinput.header, output, true);
            }
            if (kinput.metadata != nullptr && kinput.metadataSize != 0)
            {   // if kernel metadata available
                output.write("    .metadata\n", 14);
                printDisasmLongString(kinput.metadataSize, kinput.metadata, output, true);
            }
        }
        if (doDumpData && kinput.data != nullptr && kinput.dataSize != 0)
        {   // if kernel data available
            output.write("    .data\n", 10);
            printDisasmData(kinput.dataSize, kinput.data, output, true);
        }
        
        if ((flags & DISASM_CALNOTES) != 0)
            for (const CALNoteInput& calNote: kinput.calNotes)
            {
                char buf[80];
                // calNote.header fields is already in native endian
                if (calNote.header.type != 0 && calNote.header.type <= CALNOTE_ATI_MAXTYPE)
                {
                    output.write("    ", 4);
                    output.write(disasmCALNoteNamesTable[calNote.header.type-1],
                                 ::strlen(disasmCALNoteNamesTable[calNote.header.type-1]));
                }
                else
                {
                    const size_t len = itocstrCStyle(calNote.header.type, buf, 32, 16);
                    output.write("    .calnote ", 13);
                    output.write(buf, len);
                }
                
                if (calNote.data == nullptr || calNote.header.descSize==0)
                {
                    output.put('\n');
                    continue; // skip if no data
                }
                
                switch(calNote.header.type)
                {   // handle CAL note types
                    case CALNOTE_ATI_PROGINFO:
                    {
                        output.put('\n');
                        const cxuint progInfosNum =
                                calNote.header.descSize/sizeof(CALProgramInfoEntry);
                        const CALProgramInfoEntry* progInfos =
                            reinterpret_cast<const CALProgramInfoEntry*>(calNote.data);
                        ::memcpy(buf, "        .entry ", 15);
                        for (cxuint k = 0; k < progInfosNum; k++)
                        {
                            const CALProgramInfoEntry& progInfo = progInfos[k];
                            size_t bufPos = 15 + itocstrCStyle(ULEV(progInfo.address),
                                         buf+15, 32, 16, 8);
                            buf[bufPos++] = ',';
                            buf[bufPos++] = ' ';
                            bufPos += itocstrCStyle(ULEV(progInfo.value),
                                      buf+bufPos, 32, 16, 8);
                            buf[bufPos++] = '\n';
                            output.write(buf, bufPos);
                        }
                        /// rest
                        printDisasmData(calNote.header.descSize -
                                progInfosNum*sizeof(CALProgramInfoEntry),
                                calNote.data + progInfosNum*sizeof(CALProgramInfoEntry),
                                output, true);
                        break;
                    }
                    case CALNOTE_ATI_INPUTS:
                    case CALNOTE_ATI_OUTPUTS:
                    case CALNOTE_ATI_GLOBAL_BUFFERS:
                    case CALNOTE_ATI_SCRATCH_BUFFERS:
                    case CALNOTE_ATI_PERSISTENT_BUFFERS:
                        output.put('\n');
                        printDisasmDataU32(calNote.header.descSize>>2,
                               reinterpret_cast<const uint32_t*>(calNote.data),
                               output, true);
                        printDisasmData(calNote.header.descSize&3,
                               calNote.data + (calNote.header.descSize&~3U), output, true);
                        break;
                    case CALNOTE_ATI_INT32CONSTS:
                    case CALNOTE_ATI_FLOAT32CONSTS:
                    case CALNOTE_ATI_BOOL32CONSTS:
                    {
                        output.put('\n');
                        const cxuint segmentsNum =
                                calNote.header.descSize/sizeof(CALDataSegmentEntry);
                        const CALDataSegmentEntry* segments =
                                reinterpret_cast<const CALDataSegmentEntry*>(calNote.data);
                        ::memcpy(buf, "        .segment ", 17);
                        for (cxuint k = 0; k < segmentsNum; k++)
                        {
                            const CALDataSegmentEntry& segment = segments[k];
                            size_t bufPos = 17 + itocstrCStyle(
                                        ULEV(segment.offset), buf + 17, 32);
                            buf[bufPos++] = ',';
                            buf[bufPos++] = ' ';
                            bufPos += itocstrCStyle(ULEV(segment.size), buf+bufPos, 32);
                            buf[bufPos++] = '\n';
                            output.write(buf, bufPos);
                        }
                        /// rest
                        printDisasmData(calNote.header.descSize -
                                segmentsNum*sizeof(CALDataSegmentEntry),
                                calNote.data + segmentsNum*sizeof(CALDataSegmentEntry),
                                output, true);
                        break;
                    }
                    case CALNOTE_ATI_INPUT_SAMPLERS:
                    {
                        output.put('\n');
                        const cxuint samplersNum =
                                calNote.header.descSize/sizeof(CALSamplerMapEntry);
                        const CALSamplerMapEntry* samplers =
                                reinterpret_cast<const CALSamplerMapEntry*>(calNote.data);
                        ::memcpy(buf, "        .sampler ", 17);
                        for (cxuint k = 0; k < samplersNum; k++)
                        {
                            const CALSamplerMapEntry& sampler = samplers[k];
                            size_t bufPos = 17 + itocstrCStyle(
                                        ULEV(sampler.input), buf + 17, 32);
                            buf[bufPos++] = ',';
                            buf[bufPos++] = ' ';
                            bufPos += itocstrCStyle(ULEV(sampler.sampler),
                                        buf+bufPos, 32, 16);
                            buf[bufPos++] = '\n';
                            output.write(buf, bufPos);
                        }
                        /// rest
                        printDisasmData(calNote.header.descSize -
                                samplersNum*sizeof(CALSamplerMapEntry),
                                calNote.data + samplersNum*sizeof(CALSamplerMapEntry),
                                output, true);
                        break;
                    }
                    case CALNOTE_ATI_CONSTANT_BUFFERS:
                    {
                        output.put('\n');
                        const cxuint constBufMasksNum =
                                calNote.header.descSize/sizeof(CALConstantBufferMask);
                        const CALConstantBufferMask* constBufMasks =
                            reinterpret_cast<const CALConstantBufferMask*>(calNote.data);
                        ::memcpy(buf, "        .cbmask ", 16);
                        for (cxuint k = 0; k < constBufMasksNum; k++)
                        {
                            const CALConstantBufferMask& cbufMask = constBufMasks[k];
                            size_t bufPos = 16 + itocstrCStyle(
                                        ULEV(cbufMask.index), buf + 16, 32);
                            buf[bufPos++] = ',';
                            buf[bufPos++] = ' ';
                            bufPos += itocstrCStyle(ULEV(cbufMask.size), buf+bufPos, 32);
                            buf[bufPos++] = '\n';
                            output.write(buf, bufPos);
                        }
                        /// rest
                        printDisasmData(calNote.header.descSize -
                            constBufMasksNum*sizeof(CALConstantBufferMask),
                            calNote.data + constBufMasksNum*sizeof(CALConstantBufferMask),
                            output, true);
                        break;
                    }
                    case CALNOTE_ATI_EARLYEXIT:
                    case CALNOTE_ATI_CONDOUT:
                    case CALNOTE_ATI_UAV_OP_MASK:
                    case CALNOTE_ATI_UAV_MAILBOX_SIZE:
                        if (calNote.header.descSize == 4)
                        {
                            const size_t len = itocstrCStyle(
                                    ULEV(*reinterpret_cast<const uint32_t*>(
                                        calNote.data)), buf, 32);
                            output.put(' ');
                            output.write(buf, len);
                            output.put('\n');
                        }
                        else // otherwise if size is not 4 bytes
                        {
                            output.put('\n');
                            printDisasmData(calNote.header.descSize,
                                    calNote.data, output, true);
                        }
                        break;
                    case CALNOTE_ATI_UAV:
                    {
                        output.put('\n');
                        const cxuint uavsNum =
                                calNote.header.descSize/sizeof(CALUAVEntry);
                        const CALUAVEntry* uavEntries =
                            reinterpret_cast<const CALUAVEntry*>(calNote.data);
                        ::memcpy(buf, "        .entry ", 15);
                        for (cxuint k = 0; k < uavsNum; k++)
                        {
                            const CALUAVEntry& uavEntry = uavEntries[k];
                            /// uav entry format: .entry UAVID, F1, F2, TYPE
                            size_t bufPos = 15 + itocstrCStyle(
                                        ULEV(uavEntry.uavId), buf + 15, 32);
                            buf[bufPos++] = ',';
                            buf[bufPos++] = ' ';
                            bufPos += itocstrCStyle(ULEV(uavEntry.f1), buf+bufPos, 32);
                            buf[bufPos++] = ',';
                            buf[bufPos++] = ' ';
                            bufPos += itocstrCStyle(ULEV(uavEntry.f2), buf+bufPos, 32);
                            buf[bufPos++] = ',';
                            buf[bufPos++] = ' ';
                            bufPos += itocstrCStyle(ULEV(uavEntry.type), buf+bufPos, 32);
                            buf[bufPos++] = '\n';
                            output.write(buf, bufPos);
                        }
                        /// rest
                        printDisasmData(calNote.header.descSize -
                            uavsNum*sizeof(CALUAVEntry),
                            calNote.data + uavsNum*sizeof(CALUAVEntry), output, true);
                        break;
                    }
                    default:
                        output.put('\n');
                        printDisasmData(calNote.header.descSize, calNote.data,
                                        output, true);
                        break;
                }
            }
        
        if (doDumpCode && kinput.code != nullptr && kinput.codeSize != 0)
        {   // input kernel code (main disassembly)
            output.write("    .text\n", 10);
            isaDisassembler->setInput(kinput.codeSize, kinput.code);
            isaDisassembler->beforeDisassemble();
            isaDisassembler->disassemble();
            sectionCount++;
        }
    }
}

void Disassembler::disassembleAmdCL2()
{
    const bool doMetadata = ((flags & DISASM_METADATA) != 0);
    const bool doDumpData = ((flags & DISASM_DUMPDATA) != 0);
    const bool doDumpCode = ((flags & DISASM_DUMPCODE) != 0);
    const bool doSetup = ((flags & DISASM_SETUP) != 0);
    
    {
        char buf[40];
        size_t size = snprintf(buf, 40, ".driver_version %u\n",
                   amdCL2Input->driverVersion);
        output.write(buf, size);
    }
    
    if (doMetadata)
    {
        output.write(".compile_options \"", 18);
        const std::string escapedCompileOptions = 
                escapeStringCStyle(amdCL2Input->compileOptions);
        output.write(escapedCompileOptions.c_str(), escapedCompileOptions.size());
        output.write("\"\n.acl_version \"", 16);
        const std::string escapedAclVersionString =
                escapeStringCStyle(amdCL2Input->aclVersionString);
        output.write(escapedAclVersionString.c_str(), escapedAclVersionString.size());
        output.write("\"\n", 2);
    }
    if (doSetup && amdCL2Input->samplerInit!=nullptr && amdCL2Input->samplerInitSize!=0)
    {   /// sampler init entries
        output.write(".samplerinit\n", 13);
        printDisasmData(amdCL2Input->samplerInitSize, amdCL2Input->samplerInit, output);
    }
    
    if (doDumpData && amdCL2Input->globalData != nullptr &&
        amdCL2Input->globalDataSize != 0)
    {
        output.write(".globaldata\n", 12);
        output.write(".gdata:\n", 8); /// symbol used by text relocations
        printDisasmData(amdCL2Input->globalDataSize, amdCL2Input->globalData, output);
        /// put sampler relocations at global data section
        for (auto v: amdCL2Input->samplerRelocs)
        {
            output.write("    .samplerreloc ", 18);
            char buf[64];
            size_t bufPos = itocstrCStyle<size_t>(v.first, buf, 22);
            buf[bufPos++] = ',';
            buf[bufPos++] = ' ';
            bufPos += itocstrCStyle<size_t>(v.second, buf+bufPos, 22);
            buf[bufPos++] = '\n';
            output.write(buf, bufPos);
        }
    }
    if (doDumpData && amdCL2Input->rwData != nullptr &&
        amdCL2Input->rwDataSize != 0)
    {
        output.write(".data\n", 6);
        output.write(".ddata:\n", 8); /// symbol used by text relocations
        printDisasmData(amdCL2Input->rwDataSize, amdCL2Input->rwData, output);
    }
    
    if (doDumpData && amdCL2Input->bssSize)
    {
        output.write(".section .bss align=", 20);
        char buf[64];
        size_t bufPos = itocstrCStyle<size_t>(amdCL2Input->bssAlignment, buf, 22);
        buf[bufPos++] = '\n';
        output.write(buf, bufPos);
        output.write(".bdata:\n", 8); /// symbol used by text relocations
        output.write("    .skip ", 10);
        bufPos = itocstrCStyle<size_t>(amdCL2Input->bssSize, buf, 22);
        buf[bufPos++] = '\n';
        output.write(buf, bufPos);
    }
    
    for (const AmdCL2DisasmKernelInput& kinput: amdCL2Input->kernels)
    {
        output.write(".kernel ", 8);
        output.write(kinput.kernelName.c_str(), kinput.kernelName.size());
        output.put('\n');
        if (doMetadata)
        {
            if (kinput.metadata != nullptr && kinput.metadataSize != 0)
            {   // if kernel metadata available
                output.write("    .metadata\n", 14);
                printDisasmData(kinput.metadataSize, kinput.metadata, output, true);
            }
            if (kinput.isaMetadata != nullptr && kinput.isaMetadataSize != 0)
            {   // if kernel isametadata available
                output.write("    .isametadata\n", 17);
                printDisasmData(kinput.isaMetadataSize, kinput.isaMetadata, output, true);
            }
        }
        if (doSetup)
        {
            if (kinput.stub != nullptr && kinput.stubSize != 0)
            {   // if kernel setup available
                output.write("    .stub\n", 10);
                printDisasmData(kinput.stubSize, kinput.stub, output, true);
            }
            if (kinput.setup != nullptr && kinput.setupSize != 0)
            {   // if kernel setup available
                output.write("    .setup\n", 11);
                printDisasmData(kinput.setupSize, kinput.setup, output, true);
            }
        }
        if (doDumpCode && kinput.code != nullptr && kinput.codeSize != 0)
        {   // input kernel code (main disassembly)
            isaDisassembler->clearRelocations();
            isaDisassembler->addRelSymbol(".gdata");
            isaDisassembler->addRelSymbol(".ddata"); // rw data
            isaDisassembler->addRelSymbol(".bdata"); // .bss data
            for (const AmdCL2RelaEntry& entry: kinput.textRelocs)
                isaDisassembler->addRelocation(entry.offset, entry.type, 
                               cxuint(entry.symbol), entry.addend);
    
            output.write("    .text\n", 10);
            isaDisassembler->setInput(kinput.codeSize, kinput.code);
            isaDisassembler->beforeDisassemble();
            isaDisassembler->disassemble();
            sectionCount++;
        }
    }
}

static const char* galliumArgTypeNamesTbl[] =
{
    "scalar", "constant", "global", "local", "image2d_rd", "image2d_wr", "image3d_rd",
    "image3d_wr", "sampler"
};

static const char* galliumArgSemTypeNamesTbl[] =
{
    "general", "griddim", "gridoffset", "imgsize", "imgformat"
};

void Disassembler::disassembleGallium()
{
    const bool doDumpData = ((flags & DISASM_DUMPDATA) != 0);
    const bool doMetadata = ((flags & DISASM_METADATA) != 0);
    const bool doDumpCode = ((flags & DISASM_DUMPCODE) != 0);
    
    if (galliumInput->is64BitMode)
        output.write(".64bit\n", 7);
    else
        output.write(".32bit\n", 7);
    
    if (doDumpData && galliumInput->globalData != nullptr &&
        galliumInput->globalDataSize != 0)
    {   //
        output.write(".rodata\n", 8);
        printDisasmData(galliumInput->globalDataSize, galliumInput->globalData, output);
    }
    
    for (cxuint i = 0; i < galliumInput->kernels.size(); i++)
    {
        const GalliumDisasmKernelInput& kinput = galliumInput->kernels[i];
        {
            output.write(".kernel ", 8);
            output.write(kinput.kernelName.c_str(), kinput.kernelName.size());
            output.put('\n');
        }
        if (doMetadata)
        {
            char lineBuf[128];
            output.write("    .args\n", 10);
            for (const GalliumArgInfo& arg: kinput.argInfos)
            {
                ::memcpy(lineBuf, "        .arg ", 13);
                size_t pos = 13;
                // arg format: .arg TYPENAME, SIZE, TARGETSIZE, TALIGN, NUMEXT, SEMANTIC
                if (arg.type <= GalliumArgType::MAX_VALUE)
                {
                    const char* typeStr = galliumArgTypeNamesTbl[cxuint(arg.type)];
                    const size_t len = ::strlen(typeStr);
                    ::memcpy(lineBuf+pos, typeStr, len);
                    pos += len;
                }
                else
                    pos += itocstrCStyle<cxuint>(cxuint(arg.type), lineBuf+pos, 16);
                
                lineBuf[pos++] = ',';
                lineBuf[pos++] = ' ';
                pos += itocstrCStyle<uint32_t>(arg.size, lineBuf+pos, 16);
                lineBuf[pos++] = ',';
                lineBuf[pos++] = ' ';
                pos += itocstrCStyle<uint32_t>(arg.targetSize, lineBuf+pos, 16);
                lineBuf[pos++] = ',';
                lineBuf[pos++] = ' ';
                pos += itocstrCStyle<uint32_t>(arg.targetAlign, lineBuf+pos, 16);
                if (arg.signExtended)
                    ::memcpy(lineBuf+pos, ", sext, ", 8);
                else
                    ::memcpy(lineBuf+pos, ", zext, ", 8);
                pos += 8;
                if (arg.semantic <= GalliumArgSemantic::MAX_VALUE)
                {
                    const char* typeStr = galliumArgSemTypeNamesTbl[cxuint(arg.semantic)];
                    const size_t len = ::strlen(typeStr);
                    ::memcpy(lineBuf+pos, typeStr, len);
                    pos += len;
                }
                else
                    pos += itocstrCStyle<cxuint>(cxuint(arg.semantic), lineBuf+pos, 16);
                lineBuf[pos++] = '\n';
                output.write(lineBuf, pos);
            }
            /// proginfo
            output.write("    .proginfo\n", 14);
            for (const GalliumProgInfoEntry& piEntry: kinput.progInfo)
            {
                output.write("        .entry ", 15);
                char buf[32];
                size_t numSize = itocstrCStyle<uint32_t>(piEntry.address, buf, 32, 16, 8);
                output.write(buf, numSize);
                output.write(", ", 2);
                numSize = itocstrCStyle<uint32_t>(piEntry.value, buf, 32, 16, 8);
                output.write(buf, numSize);
                output.write("\n", 1);
            }
        }
        isaDisassembler->addNamedLabel(kinput.offset, kinput.kernelName);
    }
    if (doDumpCode && galliumInput->code != nullptr && galliumInput->codeSize != 0)
    {   // print text
        output.write(".text\n", 6);
        isaDisassembler->setInput(galliumInput->codeSize, galliumInput->code);
        isaDisassembler->beforeDisassemble();
        isaDisassembler->disassemble();
    }
}

void Disassembler::disassembleRawCode()
{
    if ((flags & DISASM_DUMPCODE) != 0)
    {
        output.write(".text\n", 6);
        isaDisassembler->setInput(rawInput->codeSize, rawInput->code);
        isaDisassembler->beforeDisassemble();
        isaDisassembler->disassemble();
    }
}

void Disassembler::disassemble()
{
    const std::ios::iostate oldExceptions = output.exceptions();
    output.exceptions(std::ios::failbit | std::ios::badbit);
    try
    {
    if (binaryFormat == BinaryFormat::AMD)
        output.write(".amd\n", 5);
    else if (binaryFormat == BinaryFormat::AMDCL2)
        output.write(".amdcl2\n", 8);
    else if (binaryFormat == BinaryFormat::GALLIUM) // Gallium
        output.write(".gallium\n", 9);
    else // raw code
        output.write(".rawcode\n", 9);
    
    const GPUDeviceType deviceType = getDeviceType();
    output.write(".gpu ", 5);
    const char* gpuName = getGPUDeviceTypeName(deviceType);
    output.write(gpuName, ::strlen(gpuName));
    output.put('\n');
    
    if (binaryFormat == BinaryFormat::AMD)
        disassembleAmd();
    else if (binaryFormat == BinaryFormat::AMDCL2)
        disassembleAmdCL2();
    else if (binaryFormat == BinaryFormat::GALLIUM) // Gallium
        disassembleGallium();
    else // raw code input
        disassembleRawCode();
    output.flush();
    } /* try catch */
    catch(...)
    {
        output.exceptions(oldExceptions);
        throw;
    }
    output.exceptions(oldExceptions);
}
