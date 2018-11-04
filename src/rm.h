#ifndef RM_H
#define RM_H

#include "naivedb.h"
#include "rid.h"
#include "pf.h"
#include <cstring>

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
    void Init(char* data)
    {
        memcpy(&nextFree, data, sizeof(int) * 4); // init for nextFree, preFree, freeSlotsNum, totalSlotsNum
        freeSlots = data + sizeof(int) * 4;
    }
};

class RM_BitMap
{
public:
    RM_BitMap(int numBytes, char* c);
    ~RM_BitMap();
    RC set(int bitPos, bool free);
    RC setAll(bool free);
    RC getFirstFree(int& bitPos) const;// bit position, not byte
    RC isFree(int bitPos, bool& isfree);
private:
    char* buffer;
    int size; // bytes, not bits
};

class RM_Record {
public:
    RM_Record  ();
    ~RM_Record ();
    RC SetData    (char* data, int recordSize, const RID& rid);
    RC GetData    (char *&pData) const;
    RC GetRid     (RID &rid) const;
private:
    char* data;
    int recordSize;
    RID rid;
};

class RM_FileHandle {
public:
    RM_FileHandle  ();
    ~RM_FileHandle ();
    RC GetRec         (const RID &rid, RM_Record &rec) const;
    RC InsertRec      (const char *pData, RID &rid);
    RC DeleteRec      (const RID &rid); 
    RC UpdateRec      (const RM_Record &rec);
    RC ForcePages     (PageNum pageNum = ALL_PAGES) const;
    RC Init  (RM_FileHeader header, PF_FileHandle* pffh);
    bool FileHeaderChanged() const { return fileHeaderChanged;}
    PF_FileHandle* GetPFFileHandle() const {return filehandle;}
    RC WriteFileHeaderBack();
    RC DeleteFileHandle();
private:
    bool ValidRID(const RID &rid) const;
    RC GetPageHeader(const PF_PageHandle &handle, RM_PageHeader &pageHeader) const;
    RC GetSlotPointer(const PF_PageHandle &handle, SlotNum slotNum, char*& data) const;
    RC GetFirstFreePage(PageNum &pageNum);
    RC GetFirstFreeSlot(PageNum pageNum, SlotNum &slotNum);
    RC CreateNewPage(PageNum &pageNum);
    PF_FileHandle* filehandle;
    RM_FileHeader fileheader;
    bool fileHeaderChanged;
};

#define RM_NOFREEBITMAP             (START_RM_WARN + 0) // no free slot in bitmap

#define RM_NORECORD                 (START_RM_ERR - 0)  // no record in RM_Record
#define RM_BITMAPPOSOUTOFSIZE       (START_RM_ERR - 1)  // bitmap's set position out of size
#define RM_HANDLENOTINIT            (START_RM_ERR - 2)  // RM_FileHandle have not been initialized
#define RM_HANDLEINITTWICE          (START_RM_ERR - 3)  // RM_FileHandle have been initialized twice
#define RM_INVALIDRID               (START_RM_ERR - 4)  // invalid RID
#define RM_NORECINSLOT              (START_RM_ERR - 5)  // the slot is free, no record
#define RM_SLOTNOTFREE              (START_RM_ERR - 6)  // the slot is not free, have record
#define RM_SLOTPOINTEROUTOFRANGE    (START_RM_ERR - 7)  // slot pointer out of range(offset > PF_PAGE_SIZE)
#define RM_INSERTNULLDATA           (START_RM_ERR - 8)  // insert null data
#define RM_SHOULDNOTCREATEPAGE      (START_RM_ERR - 9)  // should not create new page, because fileheader.firstFree != -1
#define RM_WRONGINLIST              (START_RM_ERR - 10) // there is something wrong with the free page list

#endif