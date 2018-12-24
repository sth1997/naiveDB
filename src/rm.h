#ifndef RM_H
#define RM_H

#include "naivedb.h"
#include "rid.h"
#include "pf.h"
#include "catalog.h"
#include <cstring>
#include <string>

struct RM_FileHeader
{
    int recordSize;
    int recordNumPerPage;
    int totalPageNum;
    int firstFree;
};
#define RM_PAGE_LIST_END  -1       // end of list of free pages

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
    RM_Record(const RM_Record& r) = delete;
    RM_Record& operator = (const RM_Record& r) = delete;
    RC SetData    (char* data, int recordSize, const RID& rid);
    RC GetData    (char *&pData) const;
    RC GetRid     (RID &rid) const;
    int GetRecordSize () { return recordSize; }
    RC IsNull(const DataRelInfo& relinfo, int bitPos, bool& isNull) const __wur;
    RC SetNull(const DataRelInfo& relinfo, int bitPos) __wur;
    RC SetNotNull(const DataRelInfo& relinfo, int bitPos) __wur;
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
    bool FileOpen() const { return bFileOpen;}
    bool FileHeaderChanged() const { return fileHeaderChanged;}
    PF_FileHandle* GetPFFileHandle() const {return filehandle;}
    RM_FileHeader GetRMFileHeader() const {return fileheader;}
    RC GetPageHeader(const PF_PageHandle &handle, RM_PageHeader &pageHeader) const;
    RC GetSlotPointer(const PF_PageHandle &handle, SlotNum slotNum, char*& data) const;
    RC WriteFileHeaderBack();
    RC DeleteFileHandle();
private:
    bool ValidRID(const RID &rid) const;
    RC GetFirstFreePage(PageNum &pageNum);
    RC GetFirstFreeSlot(PageNum pageNum, SlotNum &slotNum);
    RC CreateNewPage(PageNum &pageNum);
    PF_FileHandle* filehandle;
    RM_FileHeader fileheader;
    bool bFileOpen;
    bool fileHeaderChanged;
};

class RM_Manager {
    public:
        RM_Manager  (PF_Manager &pfm) : pf_mgr(pfm) {};            // Constructor
        ~RM_Manager () {};                           // Destructor
        RC CreateFile  (const char *fileName, int recordSize);  
        // Create a new file
        RC DestroyFile (const char *fileName);       // Destroy a file
        RC OpenFile    (const char *fileName, RM_FileHandle &fileHandle);
        // Open a file
        RC CloseFile   (RM_FileHandle &fileHandle);  // Close a file
    private:
        PF_Manager &pf_mgr;
};

class RM_FileScan {
    public:
        RM_FileScan  ();                                // Constructor
        ~RM_FileScan ();                                // Destructor
        RC OpenScan     (const RM_FileHandle &fileHandle,  // Initialize file scan
                AttrType      attrType,
                int           attrLength,
                int           attrOffset,
                CompOp        compOp,
                void          *value,
                ClientHint    pinHint = NO_HINT);
        RC GetNextRec   (RM_Record &rec);                  // Get next matching record
        RC CloseScan    ();                                // Terminate file scan
    private:
        PageNum pageNum;                                 // Current page number
        SlotNum slotNum;                                 // Current slot number

        const RM_FileHandle* rm_fhdl;                           // File handle for the file
        AttrType attrType;                                  // Attribute type
        int attrLength;                                     // Attribute length
        int attrOffset;                                     // Attribute offset
        CompOp compOp;                                      // Comparison operator
        void* value;                                        // Value to be compared
        ClientHint pinHint;                                 // Pinning hint
        int scanOpen;                                       // Flag to track is scan is open

        int getIntegerValue(char* recordData);              // Get integer attribute value
        float getFloatValue(char* recordData);              // Get float attribute value
        std::string getStringValue(char* recordData);       // Get string attribute value
        template<typename T>
            bool matchRecord(T recordValue, T givenValue);      // Match the record value with
                                                        // the given value
};

//
// Print-error function and RM return code defines
//
void RM_PrintError(RC rc);

#define RM_NOFREEBITMAP         (START_RM_WARN + 0) // no free slot in bitmap
#define RM_BADFILENAME          (START_RM_WARN + 1) // file name is null
#define RM_BADRECORDSIZE        (START_RM_WARN + 2) // record size is invalid
#define RM_INVALID_ATTRIBUTE    (START_RM_WARN + 3) // scan attribute is invalid
#define RM_INVALID_OPERATOR     (START_RM_WARN + 4) // scan operator is invalid
#define RM_ATTRIBUTE_NOT_CONSISTENT     (START_RM_WARN + 5) // attribute & attrLength not consistent
#define RM_SCAN_CLOSED          (START_RM_WARN + 6) // scan closed when get next
#define RM_FILE_NOT_OPEN          (START_RM_WARN + 7) // scan closed when get next
#define RM_EOF                  (START_RM_WARN + 8) // scan end of file
#define RM_LASTWARN             RM_EOF

#define RM_NORECORD             (START_RM_ERR - 0)  // no record in RM_Record

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
#define RM_RECORDSIZEMISMATCH       (START_RM_ERR - 11) // the record size mismatch
#define RM_PFERROR              (START_RM_ERR - 12)  // error occur in pf
#define RM_LASTERROR            RM_PFERROR

#endif
