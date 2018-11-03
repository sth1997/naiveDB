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
    RM_BitMap(int numBytes);
    ~RM_BitMap();
    void set(int bitPos, bool free);
    void setAll(bool free);
    int getFirstFree() const;
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

#define RM_NORECORD           (START_RM_ERR - 0)  // no record in RM_Record

#endif