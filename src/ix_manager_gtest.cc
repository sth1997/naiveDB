#include "gtest/gtest.h"
#include "ix.h"

#define STRLEN 29
struct TestRec {
    char str[STRLEN];
    int num;
    float r;
};

class IX_ManagerTest : public ::testing::Test {
   protected:
    IX_ManagerTest() : imm(pfm) {}
    virtual void SetUp() {
        RC rc;
        system("rm -f ix_test_file*");
        rc = imm.CreateIndex(fileName, 0, INT, 4);
        ASSERT_EQ(OK_RC, rc);
        rc = imm.OpenIndex(fileName, 0, ix_ihdl);
        ASSERT_EQ(OK_RC, rc);
    }

    virtual void TearDown() { 
        ASSERT_EQ(OK_RC, imm.DestroyIndex(fileName, 0)); 
    }

    // Declares the variables your tests want to use.
    PF_Manager pfm;
    IX_Manager imm;
    IX_IndexHandle ix_ihdl;
    const char* fileName = "ix_test_file";
};

TEST_F(IX_ManagerTest, Close) {
    ASSERT_EQ(OK_RC, imm.CloseIndex(ix_ihdl));
}
