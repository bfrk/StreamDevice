/***************************************************************
* StreamDevice Support                                         *
*                                                              *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)          *
* (C) 2005 Dirk Zimoch (dirk.zimoch@psi.ch)                    *
*                                                              *
* This is the BCD format converter of StreamDevice.            *
* Please refer to the HTML files in ../docs/ for a detailed    *
* documentation.                                               *
*                                                              *
* If you do any changes in this file, you are not allowed to   *
* redistribute it any more. If there is a bug or a missing     *
* feature, send me an email and/or your patch. If I accept     *
* your changes, they will go to the next release.              *
*                                                              *
* DISCLAIMER: If this software breaks something or harms       *
* someone, it's your problem.                                  *
*                                                              *
***************************************************************/

#include "StreamFormatConverter.h"
#include "StreamError.h"

// packed BCD (0x00 - 0x99)

class BCDConverter : public StreamFormatConverter
{
    int parse (const StreamFormat&, StreamBuffer&, const char*&, bool);
    bool printLong(const StreamFormat&, StreamBuffer&, long);
    ssize_t scanLong(const StreamFormat&, const char*, long&);
};

int BCDConverter::
parse(const StreamFormat& fmt, StreamBuffer&, const char*&, bool)
{
    return (fmt.flags & sign_flag) ? signed_format : unsigned_format;
}

bool BCDConverter::
printLong(const StreamFormat& fmt, StreamBuffer& output, long value)
{
    unsigned char bcd;
    long i, d, s;
    unsigned long prec = fmt.prec < 0 ? 2 * sizeof(value) : fmt.prec; // number of nibbles
    unsigned long width = (prec + (fmt.flags & sign_flag ? 2 : 1)) / 2;
    unsigned long val = value;

    output.append('\0', width);
    if (fmt.width > width) width = fmt.width;
    if (fmt.flags & sign_flag && value < 0)
        val = -value;
    if (fmt.flags & alt_flag)
    {
        i = -(long)width;
        d = 1;
        s = -1;
    }
    else
    {
        i = -1;
        d = -1;
        s = -(long)width;
    }
    while (width && prec)
    {
        width--;
        bcd = val%10;
        if (--prec)
        {
            --prec;
            val /= 10;
            bcd |= (val%10)<<4;
            val /= 10;
        }
        output[i] = bcd;
        i += d;
    }
    if (fmt.flags & sign_flag && value < 0) {
        output[s] |= 0xf0;
    }
    return true;
}

ssize_t BCDConverter::
scanLong(const StreamFormat& fmt, const char* input, long& value)
{
    ssize_t consumed = 0;
    long val = 0;
    unsigned char bcd1, bcd10;
    long width = fmt.width;
    if (width == 0) width = 1;
    if (fmt.flags & alt_flag)
    {
        // little endian
        int shift = 1;
        while (width--)
        {
            bcd1 = input[consumed++];
            bcd10 = bcd1>>4;
            bcd1 &= 0x0F;
            bcd10 >>= 4;
            if (bcd1 > 9 || shift * bcd1 < bcd1) break;
            if (width == 0 && fmt.flags & sign_flag)
            {
                val += bcd1 * shift;
                if (bcd10 != 0) val = -val;
                break;
            }
            if (bcd10 > 9 || shift * bcd10 < bcd10) break;
            val += (bcd1 + 10 * bcd10) * shift;
            if (shift <= 100000000) shift *= 100;
            else shift = 0;
        }
    }
    else
    {
        // big endian
        int sign = 1;
        while (width--)
        {
            long temp;
            bcd1 = input[consumed];
            bcd10 = bcd1>>4;
            bcd1 &= 0x0F;
            if (consumed == 0 && fmt.flags & sign_flag && bcd10)
            {
                sign = -1;
                bcd10 = 0;
            }
            if (bcd1 > 9 || bcd10 > 9) break;
            temp = val * 100 + (bcd1 + 10 * bcd10);
            if (temp < val)
            {
                consumed = 0;
                break;
            }
            val = temp;
            consumed++;
        }
        val *= sign;
    }
    if (consumed == 0) return -1;
    value = val;
    return consumed;
}

RegisterConverter (BCDConverter, "D");
