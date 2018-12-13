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

class RM_FileScanTest : public ::testing::Test {
   protected:
    RM_FileScanTest() : rmm(pfm)   {}
    virtual void SetUp() {
        RC rc;
        system("rm -f rm_filescan_test");
        if ((rc = rmm.CreateFile(fileName, sizeof(TestRec))) ||
            (rc = rmm.OpenFile(fileName, rm_fhdl)))
            RM_PrintError(rc);
        
    }

    virtual void TearDown() {
        rm_fscan.CloseScan();
        rmm.CloseFile(rm_fhdl);
        rmm.DestroyFile(fileName);
    }

    // Declares the variables your tests want to use.
    PF_Manager pfm;
    RM_Manager rmm;
    RM_FileHandle rm_fhdl;
    RM_FileScan rm_fscan;
    const char* fileName = "rm_filescan_test";
};

TEST_F(RM_FileScanTest, Open) {
    int a = 4;
    ASSERT_EQ(rm_fscan.OpenScan(rm_fhdl, INT, 4, 0, NO_OP, &a), OK_RC);
}

TEST_F(RM_FileScanTest, NO_OP) {
    RID rid;
    TestRec rec1(233);
    char* pData = (char*)&rec1;
    ASSERT_EQ(OK_RC, rm_fhdl.InsertRec(pData, rid));
    TestRec rec2(666);
    pData = (char*)&rec2;
    ASSERT_EQ(OK_RC, rm_fhdl.InsertRec(pData, rid));
    int a = 4;
    ASSERT_EQ(rm_fscan.OpenScan(rm_fhdl, INT, 4, 0, NO_OP, &a), OK_RC);
    RM_Record rec;
    ASSERT_EQ(rm_fscan.GetNextRec(rec), OK_RC);
    pData = NULL;
    rec.GetData(pData);
    TestRec* t_rec = (TestRec*)pData;
    ASSERT_EQ(233, t_rec->num);
    ASSERT_EQ(rm_fscan.GetNextRec(rec), OK_RC);
    pData = NULL;
    rec.GetData(pData);
    t_rec = (TestRec*)pData;
    ASSERT_EQ(666, t_rec->num);
    ASSERT_EQ(rm_fscan.GetNextRec(rec), RM_EOF);
}

TEST_F(RM_FileScanTest, EQ_OP) {
    RID rid;
    // TestRec rec1(233);
    int a = 233;
    char* pData = (char*)&a;
    ASSERT_EQ(OK_RC, rm_fhdl.InsertRec(pData, rid));
    // TestRec rec2(666);
    int b = 666;
    pData = (char*)&b;
    ASSERT_EQ(OK_RC, rm_fhdl.InsertRec(pData, rid));
    ASSERT_EQ(rm_fscan.OpenScan(rm_fhdl, INT, 4, 0, EQ_OP, &a), OK_RC);
    RM_Record rec;
    ASSERT_EQ(rm_fscan.GetNextRec(rec), OK_RC);
    pData = NULL;
    rec.GetData(pData);
    // TestRec* t_rec = (TestRec*)pData;
    int* aa = (int*) pData;
    ASSERT_EQ(233, *aa);
    ASSERT_EQ(rm_fscan.GetNextRec(rec), RM_EOF);
}
