#include "gtest/gtest.h"
#include "ix.h"

using namespace std;

// int value has 1000B, so every node can only countain few int values
#define INTLENGTH 1000
class LongInt
{
public:
    LongInt()
    {
        buf = new char[INTLENGTH];
    }
    ~LongInt()
    {
        delete []buf;
    }
    void set(int x)
    {
        int* tmp = (int*) buf;
        *tmp = x;
    }
    char* buf;
};

class IX_IndexScanTest : public ::testing::Test {
   protected:
    IX_IndexScanTest() : imm(pfm) {}
    virtual void SetUp() {
        RC rc;
        int res = system("rm -f ix_test_file*");
        rc = imm.CreateIndex(fileName, 0, INT, 1000);
        ASSERT_EQ(OK_RC, rc);
        rc = imm.OpenIndex(fileName, 0, ix_ihdl);
        ASSERT_EQ(OK_RC, rc);
    }

    virtual void TearDown() {
        ASSERT_EQ(OK_RC, ix_idsc.CloseScan());
        ASSERT_EQ(OK_RC, imm.CloseIndex(ix_ihdl));
        ASSERT_EQ(OK_RC, imm.DestroyIndex(fileName, 0));
    }

    RC insert(int x, LongInt& key)
    {
        RID rid(x, x);
        key.set(x);
        return ix_ihdl.InsertEntry((void*)key.buf, rid);
    }

    void show(RID rid) {
        PageNum pageNum;
        SlotNum slotNum;
        rid.GetPageNum(pageNum);
        rid.GetPageNum(slotNum);
        cout << pageNum << "," << slotNum << endl;
    }

    // Declares the variables your tests want to use.
    PF_Manager pfm;
    IX_Manager imm;
    IX_IndexHandle ix_ihdl;
    IX_IndexScan ix_idsc;
    const char* fileName = "ix_test_file";
};

TEST_F(IX_IndexScanTest, Open) {
    int a = 233;
    ASSERT_EQ(OK_RC, ix_idsc.OpenScan(ix_ihdl, NO_OP, &a, NO_HINT));
}

TEST_F(IX_IndexScanTest, NO_OP) {
    LongInt key;
    int cnt = 100;
    for (int i = 1; i < cnt; i++) {
        ASSERT_EQ(OK_RC, insert(i, key));
    }
    int a = 0;
    ASSERT_EQ(OK_RC, ix_idsc.OpenScan(ix_ihdl, NO_OP, &a, NO_HINT));
    RID rid;
    for (int i = 1; i < cnt; i++) {
        ASSERT_EQ(OK_RC, ix_idsc.GetNextEntry(rid));
        ASSERT_EQ(rid.pageNum, i);
        ASSERT_EQ(rid.slotNum, i);
    }
    ASSERT_EQ(IX_EOF, ix_idsc.GetNextEntry(rid));
}

/*
TEST_F(IX_IndexScanTest, EQ_OP) {
    LongInt key;
    ASSERT_EQ(OK_RC, insert(1, key));
    ASSERT_EQ(OK_RC, insert(2, key));
    ASSERT_EQ(OK_RC, insert(3, key));
    int a = 1;
    ASSERT_EQ(OK_RC, ix_idsc.OpenScan(ix_ihdl, EQ_OP, &a, NO_HINT));
    RID rid;
    ASSERT_EQ(OK_RC, ix_idsc.GetNextEntry(rid));
    show(rid);
    ASSERT_EQ(IX_EOF, ix_idsc.GetNextEntry(rid));
}

TEST_F(IX_IndexScanTest, GT_OP) {
    LongInt key;
    int cnt = 73;
    for (int i = 1; i < cnt; i++) {
        ASSERT_EQ(OK_RC, insert(i, key));
    }
    printf("%d\n", ix_ihdl.GetHeight());
    int a = 0;
    ASSERT_EQ(OK_RC, ix_idsc.OpenScan(ix_ihdl, NO_OP, &a, NO_HINT));
    RID rid;
    for (int i = 1; i < cnt; i++) {
        printf("%d\n", i);
        ASSERT_EQ(OK_RC, ix_idsc.GetNextEntry(rid));
    }
    ASSERT_EQ(IX_EOF, ix_idsc.GetNextEntry(rid));
}
*/
