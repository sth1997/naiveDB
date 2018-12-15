#include "gtest/gtest.h"
#include "ql.h"
using namespace std;

class QL_ManagerTest : public ::testing::Test {
protected:
    QL_ManagerTest() : rmm(pfm), ixm(pfm), smm(rmm, ixm){}
    virtual void SetUp() {
        
    }

    virtual void TearDown() { 
    }

    // Declares the variables your tests want to use.
    PF_Manager pfm;
    RM_Manager rmm;
    IX_Manager ixm;
    SM_Manager smm;
};

TEST_F(QL_ManagerTest, Test) {
}
