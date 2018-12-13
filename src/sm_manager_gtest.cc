#include "gtest/gtest.h"
#include "sm.h"
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

class SM_ManagerTest : public ::testing::Test {
protected:
    SM_ManagerTest() : rmm(pfm), ixm(pfm), smm(rmm, ixm){}
    virtual void SetUp() {
        dbNames.push_back(string("db1"));
        dbNames.push_back(string("db2"));
        dbNames.push_back(string("lajiyi"));
        char command[128];
        for (auto dbname : dbNames)
        {
            sprintf(command, "rm -rf %s", dbname.c_str());
            system(command);
        }
    }

    virtual void TearDown() { 
    }

    // Declares the variables your tests want to use.
    PF_Manager pfm;
    RM_Manager rmm;
    IX_Manager ixm;
    SM_Manager smm;
    vector<string> dbNames;
};

TEST_F(SM_ManagerTest, Test) {
    for (auto dbname : dbNames)
    {
        ASSERT_EQ(smm.CreateDb(dbname.c_str()), OK_RC);
        //ASSERT_EQ(smm.PrintDBs(), OK_RC);
    }
    ASSERT_EQ(smm.PrintDBs(), OK_RC);
    ASSERT_EQ(smm.CreateDb(dbNames[0].c_str()), SM_DBEXISTS);
    ASSERT_EQ(smm.PrintTables(), SM_DBNOTOPEN);
    ASSERT_EQ(smm.OpenDb(dbNames[0].c_str()), OK_RC);
    ASSERT_EQ(smm.PrintDBs(), SM_DBALREADYOPEN);
    ASSERT_EQ(smm.PrintTables(), OK_RC);
    AttrInfo attrs[2];
    attrs[0].attrName = "attr0";
    attrs[1].attrName = "attr1";
    attrs[0].attrType = INT;
    attrs[1].attrType = STRING;
    attrs[0].attrLength = 3;
    attrs[1].attrLength = 10;
    ASSERT_EQ(smm.CreateTable("testRel", 2, attrs), SM_BADTABLEPARA);
    attrs[0].attrLength = 4;
    ASSERT_EQ(smm.CreateTable("testRel", 2, attrs), OK_RC);
    ASSERT_EQ(smm.CreateTable("testRel", 2, attrs), SM_BADTABLEPARA);
    ASSERT_EQ(smm.PrintTables(), OK_RC);
    ASSERT_EQ(smm.CreateIndex("test", "attr1"), SM_NOSUCHATTR);
    ASSERT_EQ(smm.CreateIndex("testRel", "attr"), SM_NOSUCHATTR);
    ASSERT_EQ(smm.CreateIndex("testRel", "attr1"), OK_RC);
    ASSERT_EQ(smm.CreateIndex("testRel", "attr1"), SM_INDEXEXISTS);
    ASSERT_EQ(smm.PrintTable("testRel"), OK_RC);
    ASSERT_EQ(smm.DropIndex("testRel", "attr"), SM_NOSUCHATTR);
    ASSERT_EQ(smm.DropIndex("testRel", "attr0"), SM_NOSUCHINDEX);
    ASSERT_EQ(smm.DropIndex("testRel", "attr1"), OK_RC);
    ASSERT_EQ(smm.PrintTable("testRel"), OK_RC);
    ASSERT_EQ(smm.CreateIndex("testRel", "attr1"), OK_RC);
    ASSERT_EQ(smm.DropTable("testRel"), OK_RC);
    ASSERT_EQ(smm.PrintTables(), OK_RC);
    ASSERT_EQ(smm.CloseDb(), OK_RC);
    for (auto dbname : dbNames)
    {
        ASSERT_EQ(smm.DestroyDb(dbname.c_str()), OK_RC);
        ASSERT_EQ(smm.PrintDBs(), OK_RC);
    }
}
