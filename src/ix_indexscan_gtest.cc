#include "gtest/gtest.h"
#include "ix.h"

#define STRLEN 29
struct TestRec {
    char str[STRLEN];
    int num;
    float r;
};

class IX_IndexScanTest : public ::testing::Test {
   protected:
    IX_IndexScanTest() : imm(pfm) {}
    virtual void SetUp() {
        RC rc;
        system("rm -f ix_test_file*");
        rc = imm.CreateIndex(fileName, 0, INT, 4);
        ASSERT_EQ(OK_RC, rc);
        rc = imm.OpenIndex(fileName, 0, ix_ihdl);
        ASSERT_EQ(OK_RC, rc);
    }

    virtual void TearDown() {
        ASSERT_EQ(OK_RC, imm.CloseIndex(ix_ihdl));
        ASSERT_EQ(OK_RC, imm.DestroyIndex(fileName, 0));
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

// TEST_F(IX_IndexScanTest, NO_OP) {
//     int a = 233;
//     RID rid(0, 0);
//     ASSERT_EQ(OK_RC, ix_ihdl.InsertEntry(&a, rid));
// }
