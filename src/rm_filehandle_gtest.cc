#include "gtest/gtest.h"
#include "rid.h"
#include "rm.h"

#define STRLEN 29
struct TestRec {
    char str[STRLEN];
    int num;
    float r;
    TestRec(int i) {
        memset(this, 0, sizeof(TestRec));
        memset(str, ' ', STRLEN);
        sprintf(str, "a%d", i);
        num = i;
        r = (float)i;
    }
};

class RM_FileHandleTest : public ::testing::Test {
   protected:
    RM_FileHandleTest() : rmm(pfm) {}
    virtual void SetUp() {
        RC rc;
        system("rm -f rm_fhdl_test");
        rc = rmm.CreateFile(fileName, sizeof(TestRec));
        ASSERT_EQ(OK_RC, rc);
        rc = rmm.OpenFile(fileName, rm_fhdl);
        ASSERT_EQ(OK_RC, rc);
    }

    virtual void TearDown() {
        ASSERT_EQ(OK_RC, rmm.CloseFile(rm_fhdl));
        ASSERT_EQ(OK_RC, rmm.DestroyFile(fileName));
    }

    // Declares the variables your tests want to use.
    PF_Manager pfm;
    RM_Manager rmm;
    RM_FileHandle rm_fhdl;
    const char* fileName = "rm_fhdl_test";
};

TEST_F(RM_FileHandleTest, InsertRec) {
    RID rid;
    TestRec rec(1);
    char* pData = (char*)&rec;
    ASSERT_EQ(OK_RC, rm_fhdl.InsertRec(pData, rid));
}

TEST_F(RM_FileHandleTest, GetRec) {
    RID rid;
    TestRec rec(1);
    char* pData = (char*)&rec;
    ASSERT_EQ(OK_RC, rm_fhdl.InsertRec(pData, rid));

    RM_Record rm_rec;
    ASSERT_EQ(OK_RC, rm_fhdl.GetRec(rid, rm_rec));
    rm_rec.GetData(pData);
    TestRec* t_rec = (TestRec*)pData;
    ASSERT_EQ(1, t_rec->num);
}

TEST_F(RM_FileHandleTest, DeleteRec) {
    RID rid;
    TestRec rec(1);
    char* pData = (char*)&rec;
    ASSERT_EQ(OK_RC, rm_fhdl.InsertRec(pData, rid));
    ASSERT_EQ(OK_RC, rm_fhdl.DeleteRec(rid));
    RM_Record rm_rec;
    ASSERT_EQ(RM_NORECINSLOT, rm_fhdl.GetRec(rid, rm_rec));
}

TEST_F(RM_FileHandleTest, FreeSlot) {
    RID rid;
    TestRec rec(1);
    char* pData = (char*)&rec;
    ASSERT_EQ(OK_RC, rm_fhdl.InsertRec(pData, rid));
    PageNum pageNum;
    SlotNum slotNum;
    rid.GetPageNum(pageNum);
    rid.GetSlotNum(slotNum);
    PF_FileHandle* pf_fhdl = rm_fhdl.GetPFFileHandle();
    RM_PageHeader rm_phdr;
    PF_PageHandle pf_phdl;
    pf_fhdl->GetThisPage(pageNum, pf_phdl);
    rm_fhdl.GetPageHeader(pf_phdl, rm_phdr);
    RM_BitMap bitmap(rm_phdr.freeSlotsNum, rm_phdr.freeSlots);
    bool isFree;
    bitmap.isFree(slotNum, isFree);
    ASSERT_EQ(isFree, false);
    pf_fhdl->UnpinPage(pageNum);
    // printf("pn=%d,sn=%d\n", pageNum, slotNum);
}