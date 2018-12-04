#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include "ix.h"
#include "ix_internal.h"
using namespace std;

#define CHECK_NOZERO(x)            \
    {                              \
        if ((rc = x)) {            \
            PF_PrintError(rc);     \
            if (rc < 0) {          \
                return IX_PFERROR; \
            }                      \
        }                          \
    }

// Constructor
IX_Manager::IX_Manager(PF_Manager& pfm) : pf_mgr(pfm) {}

// Destructor
IX_Manager::~IX_Manager() {
    // Nothing to free
}

RC IX_Manager::CreateIndex(const char* fileName,
                           int indexNo,
                           AttrType attrType,
                           int attrLength) {
    RC rc;
    stringstream name;
    name << fileName << "." << indexNo;
    // create file
    CHECK_NOZERO(pf_mgr.CreateFile(name.str().c_str()));

    // pf_mgr -> pf_fhdl -> pf_phdl -> pData -> rm_fhdr
    PF_FileHandle pf_fhdl;
    PF_PageHandle pf_phdl;
    char* pData;
    CHECK_NOZERO(pf_mgr.OpenFile(name.str().c_str(), pf_fhdl));
    CHECK_NOZERO(pf_fhdl.AllocatePage(pf_phdl));
    CHECK_NOZERO(pf_phdl.GetData(pData));
    IX_FileHeader* rm_fhdr = (struct IX_FileHeader*)pData;
    rm_fhdr->rootPage = -1;
    rm_fhdr->attrType = attrType;
    rm_fhdr->attrLength = attrLength;
    rm_fhdr->height = 0;

    // pf_phdl -> pageNum -> mark dirty -> unpin -> close file
    PageNum pageNum;
    pf_phdl.GetPageNum(pageNum);
    CHECK_NOZERO(pf_fhdl.MarkDirty(pageNum));
    CHECK_NOZERO(pf_fhdl.UnpinPage(pageNum));
    CHECK_NOZERO(pf_mgr.CloseFile(pf_fhdl));

    return OK_RC;
}

RC IX_Manager::DestroyIndex(const char* fileName, int indexNo) {
    stringstream name;
    name << fileName << "." << indexNo;
    pf_mgr.DestroyFile(name.str().c_str());
    return OK_RC;
}

RC IX_Manager::OpenIndex(const char* fileName,
                         int indexNo,
                         IX_IndexHandle& indexHandle) {
    RC rc;
    PF_FileHandle pf_fhdl;
    char* pData;
    stringstream name;
    name << fileName << "." << indexNo;
    // pf_mgr -> pf_fhdl -> pf_phdl -> ix_fhdr
    CHECK_NOZERO(pf_mgr.OpenFile(name.str().c_str(), pf_fhdl));
    indexHandle.Open(&pf_fhdl);
    return OK_RC;
}

RC IX_Manager::CloseIndex(IX_IndexHandle& indexHandle) {
    RC rc;
    if (indexHandle.getHeaderChanged()) {
        indexHandle.SetFileHeader();
    }
    PF_FileHandle pf_fhdl = *indexHandle.getFileHandle();
    indexHandle.~IX_IndexHandle();
    CHECK_NOZERO(pf_mgr.CloseFile(pf_fhdl));
    // TODO
    return OK_RC;
}
