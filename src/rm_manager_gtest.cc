#include "gtest/gtest.h"
#include "rid.h"
#include "rm.h"

#define STRLEN 29
struct TestRec {
    char str[STRLEN];
    int num;
    float r;
};

class RM_ManagerTest : public ::testing::Test {
   protected:
    RM_ManagerTest() : rmm(pfm) {}
    virtual void SetUp() {
        RC rc;
        system("rm -f gtestfile");
        rc = rmm.CreateFile(fileName, sizeof(TestRec));
        ASSERT_EQ(OK_RC, rc);
        rc = rmm.OpenFile(fileName, rm_fhdl);
        ASSERT_EQ(OK_RC, rc);
    }

    virtual void TearDown() { ASSERT_EQ(OK_RC, rmm.DestroyFile(fileName)); }

    // Declares the variables your tests want to use.
    PF_Manager pfm;
    RM_Manager rmm;
    RM_FileHandle rm_fhdl;
    const char* fileName = "gtestfile";
};

TEST_F(RM_ManagerTest, Create) {
    ASSERT_EQ(RM_BADFILENAME, rmm.CreateFile(NULL, sizeof(TestRec)));
    ASSERT_EQ(RM_BADRECORDSIZE, rmm.CreateFile(fileName, -1));
}

TEST_F(RM_ManagerTest, Close) {
    ASSERT_EQ(OK_RC, rmm.CloseFile(rm_fhdl));
    ASSERT_EQ(RM_FILE_NOT_OPEN, rmm.CloseFile(rm_fhdl));
}
