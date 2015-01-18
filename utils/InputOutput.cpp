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
#include <algorithm>
#include <cstring>
#include <string>
#include <istream>
#include <ostream>
#include <streambuf>
#include <vector>
#include <CLRX/utils/InputOutput.h>

using namespace CLRX;

MemoryStreamBuf::MemoryStreamBuf(std::ios_base::openmode _openmode) : openMode(_openmode)
{ }

std::streambuf::pos_type MemoryStreamBuf::seekoff(std::streambuf::off_type offset,
         std::ios_base::seekdir dir, std::ios_base::openmode which)
{
    std::streambuf::pos_type pos = offset;
    if (dir == std::ios_base::cur)
    {
        if (which & std::ios_base::in)
            pos += gptr()-eback();
        else if (which & std::ios_base::out)
            pos += pptr()-pbase();
    }
    else if (dir == std::ios_base::end)
        pos += egptr()-eback();
    return seekpos(pos, which);
}

std::streambuf::pos_type MemoryStreamBuf::seekpos(std::streambuf::pos_type pos,
         std::ios_base::openmode which)
{
    if (pos < 0 || pos > egptr()-eback())
        return pos_type(off_type(-1));
    
    if (which & std::ios_base::in)
        setg(eback(), eback()+pos, epptr());
    if (which & std::ios_base::out)
        pbump(pos-(pptr()-pbase()));
    return pos;
}

std::streamsize MemoryStreamBuf::showmanyc()
{
    return egptr()-gptr();
}

std::streambuf::int_type MemoryStreamBuf::pbackfail(std::streambuf::int_type ch)
{
    if (eback() < gptr())
    {
        int_type ret = traits_type::eof();
        if (ch != ret) // not eof
        {
            const std::streambuf::char_type c = traits_type::to_char_type(ch);
            char oldPrev = gptr()[-1];
            if ((openMode & std::ios_base::out)!=0 || c != oldPrev)
            {
                gbump(-1);
                *gptr() = c;
                ret = ch;
            }
        }
        else
            gbump(-1);
        return ret;
    }
    return traits_type::eof();;
}

ArrayStreamBuf::ArrayStreamBuf(size_t size, char* buffer,
           std::ios_base::openmode openMode) : MemoryStreamBuf(openMode)
{
    setg(buffer, buffer, buffer+size);
    setp(buffer, buffer+size);
}

StringStreamBuf::StringStreamBuf(std::string& _string, std::ios_base::openmode openMode)
        : MemoryStreamBuf(openMode), string(_string)
{
    char* data = const_cast<char*>(string.data());
    const size_t size = string.size();
    setg(data, data, data+size);
    setp(data, data+size);
    if (openMode & std::ios_base::ate)
        pbump(size);
}

std::streambuf::int_type StringStreamBuf::overflow(std::streambuf::int_type ch)
{
    if (ch == traits_type::eof())
        return ch;
    const std::streambuf::char_type c = traits_type::to_char_type(ch);
    if (pptr() >= epptr())
    {
        if (string.capacity() < string.size()+1) // efficient reservation
            string.reserve(string.size() + (string.size()>>1));
        string.push_back(c);
        
        char* data = const_cast<char*>(string.data());
        // updating pointers
        const size_t size = string.size();
        const size_t readPos = gptr()-eback();
        const size_t writePos = pptr()-pbase();
        setg(data, data+readPos, data+size);
        setp(data, data+size);
        pbump(writePos);
    }
    else
        *pptr() = c;
    
    this->pbump(1);
    return ch;
}

VectorStreamBuf::VectorStreamBuf(std::vector<char>& _vector,
         std::ios_base::openmode openMode) : MemoryStreamBuf(openMode), vector(_vector)
{
    char* data = const_cast<char*>(vector.data());
    const size_t size = vector.size();
    setg(data, data, data+size);
    setp(data, data+size);
    if (openMode & std::ios_base::ate)
        pbump(size);
}

std::streambuf::int_type VectorStreamBuf::overflow(std::streambuf::int_type ch)
{
    if (ch == traits_type::eof())
        return ch;
    const std::streambuf::char_type c = traits_type::to_char_type(ch);
    if (pptr() >= epptr())
    {
        if (vector.capacity() < vector.size()+1) // efficient reservation
            vector.reserve(vector.size() + (vector.size()>>1));
        vector.push_back(c);
        
        char* data = const_cast<char*>(vector.data());
        // updating pointers
        const size_t size = vector.size();
        const size_t readPos = gptr()-eback();
        const size_t writePos = pptr()-pbase();
        setg(data, data+readPos, data+size);
        setp(data, data+size);
        pbump(writePos);
    }
    else
        *pptr() = c;
    
    this->pbump(1);
    return ch;
}

/*
 * Streams
 */

ArrayIStream::ArrayIStream(size_t size, const char* array)
        : buffer(size, const_cast<char*>(array), std::ios_base::in)
{
    rdbuf(&buffer);
}

ArrayOStream::ArrayOStream(size_t size, char* array)
        : buffer(size, array, std::ios_base::out)
{
    rdbuf(&buffer);
}

ArrayIOStream::ArrayIOStream(size_t size, char* array)
        : buffer(size, array, std::ios_base::in | std::ios_base::out)
{
    rdbuf(&buffer);
}

StringIStream::StringIStream(const std::string& string)
        : buffer(const_cast<std::string&>(string), std::ios_base::in)
{
    rdbuf(&buffer);
}

StringOStream::StringOStream(std::string& string) : buffer(string, std::ios_base::out)
{
    rdbuf(&buffer);
}

StringIOStream::StringIOStream(std::string& string)
        : buffer(string, std::ios_base::in | std::ios_base::out)
{
    rdbuf(&buffer);
}

VectorIStream::VectorIStream(const std::vector<char>& vector)
        : buffer(const_cast<std::vector<char>&>(vector), std::ios_base::in)
{
    rdbuf(&buffer);
}

VectorOStream::VectorOStream(std::vector<char>& vector)
        : buffer(vector, std::ios_base::out)
{
    rdbuf(&buffer);
}

VectorIOStream::VectorIOStream(std::vector<char>& vector)
        : buffer(vector, std::ios_base::in | std::ios_base::out)
{
    rdbuf(&buffer);
}