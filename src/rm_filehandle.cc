#include "rm.h"
#include <cstdio>
#define CHECK_NONZERO(x) { \
    if ((rc = x)) { \
        if (rc < 0) \
            return rc; \
        else \
            printf("WARNING: %d \n", rc); } \
}

RM_FileHandle::RM_FileHandle() : filehandle(NULL), fileHeaderChanged(false)
{}

RM_FileHandle::~RM_FileHandle()
{}

RC RM_FileHandle::Init(RM_FileHeader header, PF_FileHandle* pffh)
{
    if (filehandle)
        return RM_HANDLEINITTWICE;
    fileheader = header;
    filehandle = new PF_FileHandle;
    *filehandle = *pffh;
    return OK_RC;
}

bool RM_FileHandle::ValidRID(const RID &rid) const
{
    RC rc;
    PageNum pageNum;
    SlotNum slotNum;
    CHECK_NONZERO(rid.GetPageNum(pageNum));
    CHECK_NONZERO(rid.GetSlotNum(slotNum));
    if (pageNum < 0 || slotNum < 0 || 
        //pageNum >= fileheader.totalPageNum ||
        slotNum >= fileheader.recordNumPerPage)
        return false;
    return true;
}

RC RM_FileHandle::GetPageHeader(const PF_PageHandle &handle, RM_PageHeader &pageHeader) const
{
    char* data;
    RC rc;
    CHECK_NONZERO(handle.GetData(data));
    pageHeader.Init(data);
    return OK_RC;
}

RC RM_FileHandle::GetSlotPointer(const PF_PageHandle& handle, SlotNum slotNum, char*& slotPointer) const
{
    char* data;
    RC rc;
    CHECK_NONZERO(handle.GetData(data));
    size_t offset = sizeof(int) * 4 + sizeof(char) * (fileheader.recordNumPerPage >> 3) + fileheader.recordSize * slotNum;
    if (offset + fileheader.recordSize > PF_PAGE_SIZE)
        return RM_SLOTPOINTEROUTOFRANGE;
    slotPointer = data + offset;
    return OK_RC;
}

RC RM_FileHandle::GetRec(const RID &rid, RM_Record &rec) const
{
    if (!filehandle)
        return RM_HANDLENOTINIT;
    if (!ValidRID(rid))
        return RM_INVALIDRID;
    RC rc;
    PageNum pageNum;
    SlotNum slotNum;
    CHECK_NONZERO(rid.GetPageNum(pageNum));
    CHECK_NONZERO(rid.GetSlotNum(slotNum));
    PF_PageHandle pageHandle;
    CHECK_NONZERO(filehandle->GetThisPage(pageNum, pageHandle));
    RM_PageHeader pageHeader;
    CHECK_NONZERO(GetPageHeader(pageHandle, pageHeader));
    RM_BitMap bitmap(pageHeader.totalSlotsNum >> 3, pageHeader.freeSlots);
    bool isfree;
    CHECK_NONZERO(bitmap.isFree(slotNum, isfree));
    if (isfree)
        return RM_NORECINSLOT;
    char* slotPointer;
    CHECK_NONZERO(GetSlotPointer(pageHandle, slotNum, slotPointer));
    rec.SetData(slotPointer, fileheader.recordSize, rid);
    CHECK_NONZERO(filehandle->UnpinPage(pageNum));
    return OK_RC;
}

RC RM_FileHandle::CreateNewPage(PageNum &pageNum)
{
    if (!filehandle)
        return RM_HANDLENOTINIT;
    RC rc;
    PF_PageHandle pageHandle;
    CHECK_NONZERO(filehandle->AllocatePage(pageHandle));
    char* data;
    CHECK_NONZERO(pageHandle.GetData(data));
    CHECK_NONZERO(pageHandle.GetPageNum(pageNum));
    //set this new page as fileheader.firstFree
    RM_PageHeader pageHeader;
    if (fileheader.firstFree != -1)
        return RM_SHOULDNOTCREATEPAGE;
    pageHeader.nextFree = fileheader.firstFree;
    pageHeader.preFree = -1;
    pageHeader.freeSlotsNum = pageHeader.totalSlotsNum = fileheader.recordNumPerPage;
    memcpy(data, &pageHeader, sizeof(int) * 4);
    RM_BitMap bitmap(pageHeader.totalSlotsNum >> 3, data + sizeof(int) * 4);
    CHECK_NONZERO(bitmap.setAll(1));
    CHECK_NONZERO(filehandle->MarkDirty(pageNum));
    CHECK_NONZERO(filehandle->UnpinPage(pageNum));
    fileheader.firstFree = pageNum;
    fileheader.totalPageNum++;
    fileHeaderChanged = true;
    return OK_RC;
}

RC RM_FileHandle::GetFirstFreePage(PageNum &pageNum)
{
    if (!filehandle)
        return RM_HANDLENOTINIT;
    if (fileheader.firstFree == -1) // no free page, create one
    {
        RC rc;
        CHECK_NONZERO(CreateNewPage(pageNum));
        return OK_RC;
    }
    pageNum = fileheader.firstFree;
    return OK_RC;
}

//there must be at least one free slot in this page
RC RM_FileHandle::GetFirstFreeSlot(PageNum pageNum, SlotNum &slotNum)
{
    if (!filehandle)
        return RM_HANDLENOTINIT;
    RC rc;
    PF_PageHandle pageHandle;
    CHECK_NONZERO(filehandle->GetThisPage(pageNum, pageHandle));
    char* data;
    CHECK_NONZERO(pageHandle.GetData(data));
    RM_BitMap bitmap(fileheader.recordNumPerPage >> 3, data + sizeof(int) * 4);
    CHECK_NONZERO(bitmap.getFirstFree(slotNum));
    return OK_RC;
}

RC RM_FileHandle::InsertRec(const char *pData, RID &rid)
{
    if (!filehandle)
        return RM_HANDLENOTINIT;
    if (!pData)
        return RM_INSERTNULLDATA;
    RC rc;
    PF_PageHandle pageHandle;
    PageNum pageNum;
    SlotNum slotNum;
    CHECK_NONZERO(GetFirstFreePage(pageNum));
    CHECK_NONZERO(GetFirstFreeSlot(pageNum, slotNum));
    CHECK_NONZERO(filehandle->GetThisPage(pageNum, pageHandle));
    char* pageData;
    CHECK_NONZERO(pageHandle.GetData(pageData));
    RM_BitMap bitmap(fileheader.recordNumPerPage >> 3, pageData + sizeof(int) * 4);
    bool isFree;
    CHECK_NONZERO(bitmap.isFree(slotNum, isFree));
    if (!isFree)
        return RM_SLOTNOTFREE;
    CHECK_NONZERO(bitmap.set(slotNum, 0));
    char* slotPointer;
    CHECK_NONZERO(GetSlotPointer(pageHandle, slotNum, slotPointer));
    memcpy(slotPointer, pData, fileheader.recordSize);
    RM_PageHeader pageHeader;
    CHECK_NONZERO(GetPageHeader(pageHandle, pageHeader));
    pageHeader.freeSlotsNum--;
    if (pageHeader.freeSlotsNum == 0)
    {
        if (pageHeader.preFree == -1) // that is, fileheader.firstFree == pageNum
        {
            if (fileheader.firstFree != pageNum)
                return RM_WRONGINLIST;
            fileheader.firstFree = pageHeader.nextFree;
            fileHeaderChanged = true;
        }
        else
        {
            PF_PageHandle prePageHandle;
            CHECK_NONZERO(filehandle->GetThisPage(pageHeader.preFree, prePageHandle));
            char* prePageData;
            CHECK_NONZERO(prePageHandle.GetData(prePageData));
            memcpy(prePageData, &pageHeader, sizeof(int)); //pre page header.nextFree = pageHeader.nextFree
            CHECK_NONZERO(filehandle->MarkDirty(pageHeader.preFree));
            CHECK_NONZERO(filehandle->UnpinPage(pageHeader.preFree));
        }
        if (pageHeader.nextFree != -1)
        {
            PF_PageHandle nextPageHandle;
            CHECK_NONZERO(filehandle->GetThisPage(pageHeader.nextFree, nextPageHandle));
            char* nextPageData;
            CHECK_NONZERO(nextPageHandle.GetData(nextPageData));
            memcpy(nextPageData + 4, ((char*)&pageHeader) + 4, sizeof(int)); // nextPageHeader.preFree = pageHeader.preFree
            CHECK_NONZERO(filehandle->MarkDirty(pageHeader.nextFree));
            CHECK_NONZERO(filehandle->UnpinPage(pageHeader.nextFree));
        }
    }
    pageHeader.nextFree = pageHeader.preFree = -1;
    memcpy(pageData, &pageHeader, sizeof(int) * 4);
    CHECK_NONZERO(filehandle->MarkDirty(pageNum));
    CHECK_NONZERO(filehandle->UnpinPage(pageNum));
    return OK_RC;
}

RC RM_FileHandle::WriteFileHeaderBack()
{
    if (!filehandle)
        return RM_HANDLENOTINIT;
    RC rc;
    PF_PageHandle pageHandle;
    CHECK_NONZERO(filehandle->GetThisPage(0, pageHandle));
    char* data;
    CHECK_NONZERO(pageHandle.GetData(data));
    memcpy(data, &fileheader, sizeof(fileheader));
    CHECK_NONZERO(filehandle->MarkDirty(0));
    CHECK_NONZERO(filehandle->UnpinPage(0));
    return OK_RC;
}

RC RM_FileHandle::DeleteFileHandle()
{
    if (!filehandle)
        return RM_HANDLENOTINIT;
    delete filehandle;
    filehandle = NULL;
    return OK_RC;
}

RC RM_FileHandle::DeleteRec(const RID &rid)
{
    if (!filehandle)
        return RM_HANDLENOTINIT;
    if (!ValidRID(rid))
        return RM_INVALIDRID;
    RC rc;
    PageNum pageNum;
    SlotNum slotNum;
    CHECK_NONZERO(rid.GetPageNum(pageNum));
    CHECK_NONZERO(rid.GetSlotNum(slotNum));
    PF_PageHandle pageHandle;
    CHECK_NONZERO(filehandle->GetThisPage(pageNum, pageHandle));
    RM_PageHeader pageHeader;
    CHECK_NONZERO(GetPageHeader(pageHandle, pageHeader));
    RM_BitMap bitmap(pageHeader.totalSlotsNum >> 3, pageHeader.freeSlots);
    bool isfree;
    CHECK_NONZERO(bitmap.isFree(slotNum, isfree));
    if (isfree)
        return RM_NORECINSLOT;
    CHECK_NONZERO(bitmap.set(slotNum, 1));
    pageHeader.freeSlotsNum++;
    if (pageHeader.freeSlotsNum == 1)
    {
        pageHeader.nextFree = fileheader.firstFree;
        pageHeader.preFree = -1;
        if (fileheader.firstFree != -1)
        {
            PF_PageHandle nextPageHandle;
            CHECK_NONZERO(filehandle->GetThisPage(pageHeader.nextFree, nextPageHandle));
            char* nextPageData;
            CHECK_NONZERO(nextPageHandle.GetData(nextPageData));
            memcpy(nextPageData + 4, &pageNum, sizeof(int)); //nextPageHeader.preFree = pageNum
            CHECK_NONZERO(filehandle->MarkDirty(pageHeader.nextFree));
            CHECK_NONZERO(filehandle->UnpinPage(pageHeader.nextFree));
        }
        fileheader.firstFree = pageNum;
        fileHeaderChanged = true;
    }
    char* data;
    CHECK_NONZERO(pageHandle.GetData(data));
    memcpy(data, &pageHeader, sizeof(int) * 3);
    //TODO : when pageheader.freeSlotsNum == pageheader.totalSlotsNum, delete this page
    CHECK_NONZERO(filehandle->MarkDirty(pageNum));
    CHECK_NONZERO(filehandle->UnpinPage(pageNum));
    return OK_RC;
}

RC RM_FileHandle::UpdateRec(const RM_Record &rec)
{
    if (!filehandle)
        return RM_HANDLENOTINIT;
    RID rid;
    RC rc;
    CHECK_NONZERO(rec.GetRid(rid));
    if (!ValidRID(rid))
        return RM_INVALIDRID;
    PageNum pageNum;
    SlotNum slotNum;
    CHECK_NONZERO(rid.GetPageNum(pageNum));
    CHECK_NONZERO(rid.GetSlotNum(slotNum));
    PF_PageHandle pageHandle;
    CHECK_NONZERO(filehandle->GetThisPage(pageNum, pageHandle));
    RM_PageHeader pageHeader;
    CHECK_NONZERO(GetPageHeader(pageHandle, pageHeader));
    RM_BitMap bitmap(pageHeader.totalSlotsNum >> 3, pageHeader.freeSlots);
    bool isfree;
    CHECK_NONZERO(bitmap.isFree(slotNum, isfree));
    if (isfree)
        return RM_NORECINSLOT;
    char* slotPointer;
    CHECK_NONZERO(GetSlotPointer(pageHandle, slotNum, slotPointer));
    char* recData;
    CHECK_NONZERO(rec.GetData(recData));
    memcpy(slotPointer, recData, fileheader.recordSize);
    CHECK_NONZERO(filehandle->MarkDirty(pageNum));
    CHECK_NONZERO(filehandle->UnpinPage(pageNum));
    return OK_RC;
}

RC RM_FileHandle::ForcePages(PageNum pageNum) const
{
    if (!filehandle)
        return RM_HANDLENOTINIT;
    if (pageNum < 0 && pageNum != ALL_PAGES)
        return RM_INVALIDRID;
    RC rc;
    CHECK_NONZERO(filehandle->ForcePages(pageNum));
    return OK_RC;
}