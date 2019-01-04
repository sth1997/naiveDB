#include "gtest/gtest.h"
#include "ix.h"

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

class IX_IndexHandleTest : public ::testing::Test {
protected:
    IX_IndexHandleTest() : imm(pfm) {}
    virtual void SetUp() {
        RC rc;
        system("rm -f ix_test_file*");
        rc = imm.CreateIndex(fileName, 0, INT, 1000);
        ASSERT_EQ(OK_RC, rc);
        rc = imm.OpenIndex(fileName, 0, indexHandle);
        ASSERT_EQ(OK_RC, rc);
    }

    virtual void TearDown() { 
        ASSERT_EQ(OK_RC, imm.CloseIndex(indexHandle));
        ASSERT_EQ(OK_RC, imm.DestroyIndex(fileName, 0)); 
    }

    RC insert(int x, LongInt& key)
    {
        RID rid(x, x);
        key.set(x);
        return indexHandle.InsertEntry((void*)key.buf, rid);
    }

    RC remove(int x, LongInt& key)
    {
        RID rid(x, x);
        key.set(x);
        return indexHandle.DeleteEntry((void*)key.buf, rid);
    }

    // Declares the variables your tests want to use.
    PF_Manager pfm;
    IX_Manager imm;
    IX_IndexHandle indexHandle;
    const char* fileName = "ix_test_file";
};

TEST_F(IX_IndexHandleTest, TestInsert) {
    ASSERT_EQ(indexHandle.GetHeight(), 1);
    LongInt key;

    ASSERT_EQ(insert(1, key), OK_RC);
    ASSERT_EQ(insert(1, key), IX_ENTRYEXISTS);
    ASSERT_EQ(insert(3, key), OK_RC);
    ASSERT_EQ(insert(5, key), OK_RC);
    ASSERT_EQ(insert(7, key), OK_RC);
    ASSERT_EQ(indexHandle.GetHeight(), 1);
    // split, height + 1
    ASSERT_EQ(insert(9, key), OK_RC);
    ASSERT_EQ(indexHandle.GetHeight(), 2);

    BTreeNode* rootNode = indexHandle.GetRootNode();
    ASSERT_EQ(rootNode->getNum(), 2);
    ASSERT_EQ(*(int*) rootNode->getKey(0), 3);
    ASSERT_EQ(*(int*) rootNode->getKey(1), 9);
    ASSERT_EQ(imm.CloseIndex(indexHandle), OK_RC);
    ASSERT_EQ(imm.OpenIndex(fileName, 0, indexHandle), OK_RC);
    rootNode = indexHandle.GetRootNode();
    ASSERT_EQ(*(int*)indexHandle.GetLargestKey(), 9);
    ASSERT_EQ(remove(9, key), OK_RC);
    ASSERT_EQ(*(int*)indexHandle.GetLargestKey(), 7);
    ASSERT_EQ(remove(9, key), IX_ENTRYNOTEXIST);
    ASSERT_EQ(remove(5, key), OK_RC);
    ASSERT_EQ(remove(7, key), OK_RC);
    ASSERT_EQ(indexHandle.GetHeight(), 2);
    ASSERT_EQ(rootNode->getNum(), 1);
    ASSERT_EQ(remove(1, key), OK_RC);
    ASSERT_EQ(remove(3, key), OK_RC);
    ASSERT_EQ(indexHandle.GetHeight(), 1);
    ASSERT_EQ(rootNode->getNum(), 0);
}

TEST_F(IX_IndexHandleTest, TestInsert1) {
    ASSERT_EQ(indexHandle.GetHeight(), 1);
    LongInt key;
    int cnt = 100;

    for (int i = 1; i < cnt; i++) {
        ASSERT_EQ(insert(i, key), OK_RC);
    }
    ASSERT_EQ(indexHandle.GetHeight(), 6);
}

TEST_F(IX_IndexHandleTest, TestInsert2) {
    ASSERT_EQ(indexHandle.GetHeight(), 1);
    LongInt key;

    ASSERT_EQ(insert(1, key), OK_RC);
    ASSERT_EQ(insert(1, key), IX_ENTRYEXISTS);
    ASSERT_EQ(insert(3, key), OK_RC);
    ASSERT_EQ(insert(5, key), OK_RC);
    ASSERT_EQ(insert(7, key), OK_RC);
    ASSERT_EQ(indexHandle.GetHeight(), 1);
    // split, height + 1
    ASSERT_EQ(insert(9, key), OK_RC);
    ASSERT_EQ(indexHandle.GetHeight(), 2);

    ASSERT_EQ(remove(3, key), OK_RC);
    ASSERT_EQ(insert(3, key), OK_RC);
}