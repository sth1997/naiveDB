#ifndef RM_H
#define RM_H

#include "naivedb.h"
#include "rid.h"

struct RM_FileHeader
{
    int recordSize;
    int recordNumPerPage;
    int totalPageNum;
    int firstFree;
};

struct RM_PageHeader
{
    int nextFree;
    int preFree;
    int freeSlotsNum;
    int totalSlotsNum; // 1 bytes = 8 slots
    char* freeSlots; //pointer to the bitmap
};

class RM_BitMap
{
public:
    RM_BitMap(int numBytes, bool free);
    ~RM_BitMap();
    RC set(int bitPos, bool free);
    RC setAll(bool free);
    RC getFirstFree(int& pos) const;// bit position, not byte
private:
    char* buffer;
    int size; // bytes, not bits
};

class RM_Record {
public:
    RM_Record  ();
    ~RM_Record ();
    RC GetData    (char *&pData) const;
    RC GetRid     (RID &rid) const;
private:
    char* data;
    int recordSize;
    RID rid;
};

#define RM_NOFREEBITMAP             (START_RM_WARN + 0) // no free slot in bitmap

#define RM_NORECORD                 (START_RM_ERR - 0)  // no record in RM_Record
#define RM_BITMAPPOSOUTOFSIZE       (START_RM_ERR - 1)  // bitmap's set position out of size

#endif