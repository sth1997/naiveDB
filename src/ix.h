//
// ix.h
//
//   Index Manager Component Interface
//

#ifndef IX_H
#define IX_H

// Please do not include any other files than the ones below in this file.

#include "naivedb.h"  // Please don't change these lines
#include "rid.h"  // Please don't change these lines
#include "pf.h"
#include "btreenode.h"

struct IX_FileHeader
{
    PageNum rootPage; //init = -1
    AttrType attrType;
    int attrLength;
    int height; //init = 0
};

class IX_IndexHandle {
    friend class IX_Manager;
    friend class IX_IndexScan;
public:
    IX_IndexHandle();
    ~IX_IndexHandle();
    RC Open(PF_FileHandle* handle);

    // Insert a new index entry
    RC InsertEntry(void *pData, const RID &rid);

    // Delete a new index entry
    RC DeleteEntry(void *pData, const RID &rid);

    // Force index files to disk
    RC ForcePages();

    RC SetFileHeader() const;
    RC FindLeaf(const void* pData, const RID& rid, BTreeNode* &node);
    int GetHeight() const
    {
        return fileHeader.height;
    }
    RC FindLargestLeaf(BTreeNode* &node);
    RC FindSmallestLeaf(BTreeNode* &node);
    RC FetchNode(PageNum page, BTreeNode* &node) const;
    AttrType GetAttrType() const { return fileHeader.attrType; }
    int GetAttrLength() const { return fileHeader.attrLength; }

private:
    RC AllocatePage(PageNum& pageNum);
    RC GetFileHeader();
    RC IsValid() const;
    RC DeleteNode(BTreeNode* &node);
    RC SetHeight(int h);
    RC DisposePage(PageNum pageNum);
    bool fileOpen;
    bool headerChanged;
    BTreeNode* rootNode;
    BTreeNode** path;
    int* childPos;
    IX_FileHeader fileHeader;
    PF_FileHandle* fileHandle;
    char* largestKey;
    RID largestRID;
};

class IX_Manager {
public:
    IX_Manager(PF_Manager &pfm);
    ~IX_Manager();

    // Create a new Index
    RC CreateIndex(const char *fileName, int indexNo,
                   AttrType attrType, int attrLength);

    // Destroy and Index
    RC DestroyIndex(const char *fileName, int indexNo);

    // Open an Index
    RC OpenIndex(const char *fileName, int indexNo,
                 IX_IndexHandle &indexHandle);

    // Close an Index
    RC CloseIndex(IX_IndexHandle &indexHandle);

private:
    PF_Manager &pfm;
};

class IX_IndexScan {
public:
    IX_IndexScan();
    ~IX_IndexScan();

    // Open index scan
    RC OpenScan(const IX_IndexHandle &indexHandle,
                CompOp compOp,
                void *value,
                ClientHint  pinHint = NO_HINT);

    // Get the next matching entry return IX_EOF if no more matching
    // entries.
    RC GetNextEntry(RID &rid);

    // Close index scan
    RC CloseScan();

private:
    int scanOpen;                           // Flag to track if scan open
};

#define IX_SCAN_CLOSED          (START_IX_WARN + 0) // scan is closed
#define IX_LASTWARN             IX_SCAN_CLOSED
#define IX_NODEISFULL           (START_IX_WARN + 1) // node is full, cannot insert an entry
#define IX_NODEISEMPTY          (START_IX_WARN + 2) // node is empty, cannot delete an entry
#define IX_ENTRYEXISTS          (START_IX_WARN + 3) // entry exists
#define IX_ENTRYNOTEXIST        (START_IX_WARN + 4) // entry doesn't exist

#define IX_ERROR                (START_IX_ERR - 0) // error
#define IX_LASTERROR            IX_ERROR
#define IX_ALREADYINNODE        (START_IX_ERR - 1) // this key is already in this node
#define IX_NOTINORDER           (START_IX_ERR - 2) // the keys are not in order after insertion
#define IX_NOTINNODE            (START_IX_ERR - 3) // the key is not in the node
#define IX_SPLITTOOEARLY        (START_IX_ERR - 4) // node split when keyNum < maxKeyNum
#define IX_SPLITWITHNOTEMPTY    (START_IX_ERR - 5) // the node that one node split with is not empty
#define IX_REMOVEOUTOFRANGE     (START_IX_ERR - 6) // in function remove, pos >= keyNum
#define IX_OPENFILETWICE        (START_IX_ERR - 7) // open one file twice
#define IX_INVALIDFILEHANDLE    (START_IX_ERR - 8) // the input PF_FileHandle is invalid
#define IX_INVALID              (START_IX_ERR - 9) // invalid
#define IX_NULLKEYDATA          (START_IX_ERR - 10)// the key data pointer = NULL
#endif
