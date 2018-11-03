#include "rm.h"
#include <cstring>


RM_BitMap::RM_BitMap(int numBytes, bool free)
{
    this->size = numBytes;
    this->buffer = new char[numBytes];
    memset(this->buffer, free, numBytes);
}

RM_BitMap::~RM_BitMap()
{
    if (buffer)
        delete []buffer;
}

RC RM_BitMap::set(int bitPos, bool free)
{
    if ((bitPos / 8) >= size)
        return RM_BITMAPPOSOUTOFSIZE;
    char& tmp = buffer[bitPos / 8];
    char allFree = (1 << 8) - 1;
    int posInByte = bitPos & 7; // = bitPos % 8
    if (free)
        tmp |= (1 << posInByte);
    else
        tmp &= allFree - (1 << posInByte);
    return OK_RC;
}

RC RM_BitMap::setAll(bool free)
{
    memset(buffer, free, size);
    return OK_RC;
}

RC RM_BitMap::getFirstFree(int& pos) const
{
    for (int i = 0; i < size; ++i)
        if (buffer[i])
        {
            char tmp = buffer[i] & (-buffer[i]);
            int res = 0;
            while (tmp)
            {
                res++;
                tmp >>= 1;
            }
            pos = i * 8 + res - 1;
            return OK_RC;
        }
    return RM_NOFREEBITMAP;
}