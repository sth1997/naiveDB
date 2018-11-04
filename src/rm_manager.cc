#include "pf.h"
#include "rm_internal.h"

#define CHECK_NOZERO(x) { \
    if ((rc = x)) { \
        PF_PrintError(rc); \
        if (rc < 0) { \
            return RM_PFERROR; \
        } \
    } \
}

RC RM_Manager::CreateFile(const char* fileName, int recordSize) {
    // basic check
    if (fileName == NULL) {
        return RM_BADFILENAME;
    }

    // TODO: cal bit-wise rather than char-wise
    int recordNumPerPage = (PF_PAGE_SIZE - 4 * sizeof(int)) / (8 * recordSize + 1) * 8;
    // record size too big or <= 0
    if (recordSize > PF_PAGE_SIZE - (int) sizeof(RM_FileHeader) || recordNumPerPage < 1 || recordSize <= 0) {
        return RM_BADRECORDSIZE;
    }

    RC rc;
    // create file
    CHECK_NOZERO(pf_mgr.CreateFile(fileName));

    // pf_mgr -> pf_fhdl -> pf_phdl -> pData -> rm_fhdr
    PF_FileHandle pf_fhdl;
    PF_PageHandle pf_phdl;
    char* pData;
    CHECK_NOZERO(pf_mgr.OpenFile(fileName, pf_fhdl));
    CHECK_NOZERO(pf_fhdl.AllocatePage(pf_phdl));
    CHECK_NOZERO(pf_phdl.GetData(pData));
    RM_FileHeader* rm_fhdr = (struct RM_FileHeader*) pData;
    rm_fhdr->recordSize = recordSize;
    rm_fhdr->recordNumPerPage = recordNumPerPage;
    rm_fhdr->totalPageNum = 1;
    rm_fhdr->firstFree = RM_PAGE_LIST_END;

    // pf_phdl -> pageNum -> mark dirty -> unpin -> close file
    PageNum pageNum;
    pf_phdl.GetPageNum(pageNum);
    CHECK_NOZERO(pf_fhdl.MarkDirty(pageNum));
    CHECK_NOZERO(pf_fhdl.UnpinPage(pageNum));
    CHECK_NOZERO(pf_mgr.CloseFile(pf_fhdl));

    return OK_RC;
}

RC RM_Manager::DestroyFile(const char* fileName) {
    // basic check
    if (fileName == NULL) {
        return RM_BADFILENAME;
    }

    RC rc;
    // destroy file
    CHECK_NOZERO(pf_mgr.DestroyFile(fileName));

    return OK_RC;
}

RC RM_Manager::OpenFile(const char* fileName, RM_FileHandle& rm_fhdl) {
    RC rc;
    PF_FileHandle pf_fhdl;
    PF_PageHandle pf_phdl;
    char* pData;
    // open file
    // pf_mgr -> pf_fhdl -> pf_phdl -> pData -> rm_fhdr
    CHECK_NOZERO(pf_mgr.OpenFile(fileName, pf_fhdl));
    CHECK_NOZERO(pf_mgr.OpenFile(fileName, pf_fhdl));
    CHECK_NOZERO(pf_fhdl.GetFirstPage(pf_phdl));
    CHECK_NOZERO(pf_phdl.GetData(pData));
    struct RM_FileHeader *rm_fhdr = (struct RM_FileHeader*) pData;
    rm_fhdl.Init(*rm_fhdr, &pf_fhdl);

    // pf_phdl -> pageNum -> unpin
    PageNum pageNum;
    CHECK_NOZERO(pf_phdl.GetPageNum(pageNum));
    CHECK_NOZERO(pf_fhdl.UnpinPage(pageNum));

    return OK_RC;
}

RC RM_Manager::CloseFile(RM_FileHandle& rm_fhdl) {
    RC rc;
    // rm_fhdl -> pf_fhdl
    if (rm_fhdl.FileHeaderChanged()) {
        if ((rc = rm_fhdl.WriteFileHeaderBack())) {
            return rc; 
        }
    }
    PF_FileHandle* pf_fhdl = rm_fhdl.GetPFFileHandle();
    pf_fhdl->ForcePages();
    CHECK_NOZERO(pf_mgr.CloseFile(*pf_fhdl));
    rm_fhdl.DeleteFileHandle();

    return OK_RC;
}
