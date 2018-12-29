#include "gtest/gtest.h"
#include "ql.h"
using namespace std;

class QL_ManagerTest : public ::testing::Test {
protected:
    QL_ManagerTest() : rmm(pfm), ixm(pfm), smm(rmm, ixm), qmm(smm, ixm, rmm) {
    }
    virtual void SetUp() {
        nAttrs = 3;
        nRecords = 3;
        AttrInfo attrs[nAttrs];
        for (int i = 0; i < nAttrs; i++) {
            attrs[i].attrLength = 4;
            attrs[i].attrName = new char[12];
            string an = "attr" + to_string(i);
            strcpy(attrs[i].attrName, an.c_str());
            attrs[i].attrType = INT;
            attrs[i].couldBeNULL = i % 2 == 0 ? true : false;
        }
        system("rm -rf test");
        ASSERT_EQ(smm.CreateDb("test"), OK_RC);
        ASSERT_EQ(smm.OpenDb("test"), OK_RC);
        
        for (int i = 1; i <= nAttrs; i++) {
            string rn = "testRel"+ to_string(i);
            ASSERT_EQ(smm.CreateTable(rn.c_str(), i, attrs), OK_RC);
        }
        for (int i = 0; i < nAttrs; i++) {
            delete []attrs[i].attrName;
        }
    }

    virtual void TearDown() {
        ASSERT_EQ(smm.CloseDb(), OK_RC);
        ASSERT_EQ(smm.DestroyDb("test"), OK_RC);
    }

    void insert(int n, int ptrs[]) {
        Value values[n];
        for (int i = 0; i < n; i++) {
            values[i].data = &ptrs[i];
            values[i].type = INT;
        }
        string rn = "testRel"+ to_string(n);
        ASSERT_EQ(qmm.Insert(rn.c_str(), n, values), OK_RC);
    }

    // Declares the variables your tests want to use.
    PF_Manager pfm;
    RM_Manager rmm;
    IX_Manager ixm;
    SM_Manager smm;
    QL_Manager qmm;
    int nAttrs, nRecords;
};

TEST_F(QL_ManagerTest, Insert) {
    int ptrs[nAttrs];
    for (int i = 0; i < nAttrs; i++) {
        ptrs[i] = i;
    }
    insert(nAttrs, ptrs);
    RM_FileHandle rm_fhdl;
    RM_FileScan rm_fscan;
    ASSERT_EQ(rmm.OpenFile("testRel3", rm_fhdl), OK_RC);
    ASSERT_EQ(rm_fscan.OpenScan(rm_fhdl, INT, 4, 0, NO_OP, ptrs), OK_RC);
    RM_Record rec;
    ASSERT_EQ(rm_fscan.GetNextRec(rec), OK_RC);
    char* pData = NULL;
    rec.GetData(pData);
    int tmp;
    for (int i = 0; i < nAttrs; i++) {
        memcpy(&tmp, pData+4*i, 4);
        ASSERT_EQ(tmp, ptrs[i]);
    }
    ASSERT_EQ(rm_fscan.GetNextRec(rec), RM_EOF);
    int n = 4;
    Value values[n];
    for (int i = 0; i < n; i++) {
        values[i].data = &ptrs[i];
        values[i].type = INT;
    }
    ASSERT_EQ(qmm.Insert("testRel3", n, values), QL_INCONSISTENT_VALUE_AMOUNT);
}

TEST_F(QL_ManagerTest, Select) {
    int ptrs[nAttrs];
    for (int i = 0; i < nAttrs; i++) {
        ptrs[i] = i;
    }
    for (int i = 1; i <= nRecords; i++) {
        ptrs[0] = i;
        insert(nAttrs, ptrs);
    }
    int nSelAttrs = 2;
    RelAttr selAttrs[2];
    selAttrs[0].relName = "testRel3";
    selAttrs[0].attrName = "attr0";
    selAttrs[1].relName = "testRel3";
    selAttrs[1].attrName = "attr1";
    int nRelations = 1;
    char* relations[1];
    relations[0] = "testRel3";
    int nConditions = 2;
    Condition conditions[2];
    conditions[0].lhsAttr = selAttrs[0];
    conditions[0].op = GE_OP;
    conditions[0].rhsAttr.relName = "testRel3";
    conditions[0].rhsAttr.attrName = "attr2";
    conditions[0].bRhsIsAttr = true;
    conditions[1].lhsAttr = selAttrs[0];
    conditions[1].op = LE_OP;
    int a = 5;
    conditions[1].rhsValue.data = &a;
    conditions[1].rhsValue.type = INT;
    conditions[1].bRhsIsAttr = false;

    ASSERT_EQ(qmm.Select(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions), OK_RC);
}

TEST_F(QL_ManagerTest, SelectStar) {
    int ptrs[nAttrs];
    for (int i = 0; i < nAttrs; i++) {
        ptrs[i] = i;
    }
    for (int i = 1; i <= nRecords; i++) {
        ptrs[0] = i;
        insert(nAttrs, ptrs);
    }
    int nSelAttrs = 1;
    RelAttr selAttrs[1];
    selAttrs[0].attrName = "*";
    selAttrs[0].relName = NULL;
    int nRelations = 1;
    char* relations[1];
    relations[0] = "testRel3";
    int nConditions = 1;
    Condition conditions[1];
    conditions[0].lhsAttr.relName = "testRel3";
    conditions[0].lhsAttr.attrName = "attr0";
    conditions[0].op = GE_OP;
    conditions[0].rhsAttr.relName = "testRel3";
    conditions[0].rhsAttr.attrName = "attr2";
    conditions[0].bRhsIsAttr = true;
    ASSERT_EQ(qmm.Select(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions), OK_RC);
}

TEST_F(QL_ManagerTest, SelectJoin) {
    int ptrs[nAttrs];
    for (int i = 0; i < nAttrs; i++) {
        ptrs[i] = i * 2;
    }
    for (int i = 1; i <= nRecords; i++) {
        ptrs[0] = i;
        insert(nAttrs, ptrs);
    }
    for (int i = 1; i <= nRecords; i++) {
        ptrs[0] = i;
        insert(nAttrs - 1, ptrs);
    }
    int nSelAttrs = 1;
    RelAttr selAttrs[1];
    selAttrs[0].attrName = "*";
    selAttrs[0].relName = NULL;
    int nRelations = 2;
    char* relations[2];
    relations[0] = "testRel3";
    relations[1] = "testRel2";
    int nConditions = 1;
    Condition conditions[1];
    conditions[0].lhsAttr.relName = "testRel3";
    conditions[0].lhsAttr.attrName = "attr0";
    conditions[0].op = LE_OP;
    conditions[0].rhsAttr.relName = "testRel2";
    conditions[0].rhsAttr.attrName = "attr1";
    conditions[0].bRhsIsAttr = true;

    ASSERT_EQ(qmm.Select(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions), OK_RC);
}

TEST_F(QL_ManagerTest, Delete) {
    int ptrs[nAttrs];
    for (int i = 0; i < nAttrs; i++) {
        ptrs[i] = i;
    }
    for (int i = 1; i <= nRecords; i++) {
        ptrs[0] = i;
        insert(nAttrs, ptrs);
    }
    char* relName = "testRel3";
    int nConditions = 1;
    Condition conditions[1];
    conditions[0].lhsAttr.relName = "testRel3";
    conditions[0].lhsAttr.attrName = "attr0";
    conditions[0].op = EQ_OP;
    conditions[0].rhsAttr.relName = "testRel3";
    conditions[0].rhsAttr.attrName = "attr2";
    conditions[0].bRhsIsAttr = true;
    ASSERT_EQ(qmm.Delete(relName, nConditions, conditions), OK_RC);
    int nSelAttrs = 1;
    RelAttr selAttrs[1];
    selAttrs[0].attrName = "*";
    selAttrs[0].relName = NULL;
    int nRelations = 1;
    char* relations[1];
    relations[0] = "testRel3";
    nConditions = 0;
    ASSERT_EQ(qmm.Select(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions), OK_RC);
}

TEST_F(QL_ManagerTest, Update) {
    int ptrs[nAttrs];
    for (int i = 0; i < nAttrs; i++) {
        ptrs[i] = i;
    }
    for (int i = 1; i <= nRecords; i++) {
        ptrs[0] = i;
        insert(nAttrs, ptrs);
    }
    char* relName = "testRel3";
    RelAttr updAttr;
    updAttr.relName = "testRel3";
    updAttr.attrName = "attr0";
    int bIsValue = true;
    Value rhsValue;
    int a = 5;
    rhsValue.data = &a;
    rhsValue.type = INT;
    int nConditions = 1;
    Condition conditions[1];
    conditions[0].lhsAttr.relName = "testRel3";
    conditions[0].lhsAttr.attrName = "attr0";
    conditions[0].op = EQ_OP;
    conditions[0].rhsAttr.relName = "testRel3";
    conditions[0].rhsAttr.attrName = "attr2";
    conditions[0].bRhsIsAttr = true;
    ASSERT_EQ(qmm.Update(relName, updAttr, bIsValue, updAttr, rhsValue, nConditions, conditions), OK_RC);
    int nSelAttrs = 1;
    RelAttr selAttrs[1];
    selAttrs[0].attrName = "*";
    int nRelations = 1;
    char* relations[1];
    relations[0] = "testRel3";
    nConditions = 0;
    ASSERT_EQ(qmm.Select(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions), OK_RC);
}

TEST_F(QL_ManagerTest, UpdateBatch) {
    int ptrs[nAttrs];
    for (int i = 0; i < nAttrs; i++) {
        ptrs[i] = i;
    }
    for (int i = 1; i <= nRecords; i++) {
        ptrs[0] = i;
        insert(nAttrs, ptrs);
    }
    char* relName = "testRel3";
    int nColumns = 2;
    char* columnNames[2] = {"attr0", "attr1"};
    Value rhsValue[2];
    int a = 5;
    rhsValue[0].data = &a;
    rhsValue[0].type = INT;
    rhsValue[1].data = &a;
    rhsValue[1].type = INT;
    int nConditions = 1;
    Condition conditions[1];
    conditions[0].lhsAttr.relName = "testRel3";
    conditions[0].lhsAttr.attrName = "attr0";
    conditions[0].op = EQ_OP;
    conditions[0].rhsAttr.relName = "testRel3";
    conditions[0].rhsAttr.attrName = "attr2";
    conditions[0].bRhsIsAttr = true;
    ASSERT_EQ(qmm.Update(relName, nColumns, columnNames, rhsValue, nConditions, conditions), OK_RC);
    int nSelAttrs = 1;
    RelAttr selAttrs[1];
    selAttrs[0].attrName = "*";
    int nRelations = 1;
    char* relations[1];
    relations[0] = "testRel3";
    nConditions = 0;
    ASSERT_EQ(qmm.Select(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions), OK_RC);
}

TEST_F(QL_ManagerTest, InsertNULL) {
    Value values[nAttrs];
    int ptrs[nAttrs];
    for (int i = 0; i < nAttrs; i++) {
        ptrs[i] = i;
        values[i].data = &ptrs[i];
        values[i].type = INT;
    }
    ASSERT_EQ(qmm.Insert("testRel3", nAttrs, values), OK_RC);
    values[0].type = NULLTYPE;
    ASSERT_EQ(qmm.Insert("testRel3", nAttrs, values), OK_RC);
    values[1].type = NULLTYPE;
    ASSERT_EQ(qmm.Insert("testRel3", nAttrs, values), QL_ATTR_CANT_BE_NULL);
}

TEST_F(QL_ManagerTest, SelectNULL) {
    int ptrs[nAttrs];
    Value values[nAttrs];

    for (int i = 0; i < nAttrs; i++) {
        ptrs[i] = i;
    }
    for (int i = 1; i <= nRecords; i++) {
        ptrs[0] = i;
        insert(nAttrs, ptrs);
    }
    for (int i = 0; i < nAttrs; i++) {
        values[i].data = &ptrs[i];
        values[i].type = INT;
    }

    values[0].type = NULLTYPE;
    ASSERT_EQ(qmm.Insert("testRel3", nAttrs, values), OK_RC);

    int nSelAttrs = 1;
    RelAttr selAttrs[1];
    selAttrs[0].attrName = "*";
    selAttrs[0].relName = NULL;
    int nRelations = 1;
    char* relations[1];
    relations[0] = "testRel3";
    int nConditions = 1;
    Condition conditions[2];
    conditions[0].lhsAttr.relName = "testRel3";
    conditions[0].lhsAttr.attrName = "attr0";
    conditions[0].isNULL = true;
    conditions[0].isNotNULL = false;

    ASSERT_EQ(qmm.Select(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions), OK_RC);

    conditions[0].isNULL = false;
    conditions[0].isNotNULL = true;

    ASSERT_EQ(qmm.Select(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions), OK_RC);
}

TEST_F(QL_ManagerTest, DeleteNULL) {
    int ptrs[nAttrs];
    Value values[nAttrs];

    for (int i = 0; i < nAttrs; i++) {
        ptrs[i] = i;
    }
    for (int i = 1; i <= nRecords; i++) {
        ptrs[0] = i;
        insert(nAttrs, ptrs);
    }
    for (int i = 0; i < nAttrs; i++) {
        values[i].data = &ptrs[i];
        values[i].type = INT;
    }

    values[0].type = NULLTYPE;
    ASSERT_EQ(qmm.Insert("testRel3", nAttrs, values), OK_RC);

    int nSelAttrs = 1;
    RelAttr selAttrs[1];
    selAttrs[0].attrName = "*";
    selAttrs[0].relName = NULL;
    int nRelations = 1;
    char* relations[1];
    relations[0] = "testRel3";
    int nConditions = 1;
    Condition conditions[2];
    conditions[0].lhsAttr.relName = "testRel3";
    conditions[0].lhsAttr.attrName = "attr0";
    conditions[0].isNULL = true;
    conditions[0].isNotNULL = false;
    ASSERT_EQ(qmm.Select(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions), OK_RC);
    ASSERT_EQ(qmm.Delete(relations[0], nConditions, conditions), OK_RC);
    ASSERT_EQ(qmm.Select(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions), OK_RC);
}

TEST_F(QL_ManagerTest, UpdateNULL) {
    int ptrs[nAttrs];
    Value values[nAttrs];

    for (int i = 0; i < nAttrs; i++) {
        ptrs[i] = i;
    }
    for (int i = 1; i <= nRecords; i++) {
        ptrs[0] = i;
        insert(nAttrs, ptrs);
    }
    for (int i = 0; i < nAttrs; i++) {
        values[i].data = &ptrs[i];
        values[i].type = INT;
    }

    values[0].type = NULLTYPE;
    ASSERT_EQ(qmm.Insert("testRel3", nAttrs, values), OK_RC);

    int nSelAttrs = 1;
    RelAttr selAttrs[1];
    selAttrs[0].attrName = "*";
    selAttrs[0].relName = NULL;
    char* relName = "testRel3";
    int nColumns = 2;
    char* columnNames[2] = {"attr0", "attr1"};
    Value rhsValue[2];
    int a = 5;
    rhsValue[0].data = &a;
    rhsValue[0].type = INT;
    rhsValue[1].data = &a;
    rhsValue[1].type = INT;
    int nRelations = 1;
    char* relations[1];
    relations[0] = "testRel3";
    int nConditions = 1;
    Condition conditions[2];
    conditions[0].lhsAttr.relName = "testRel3";
    conditions[0].lhsAttr.attrName = "attr0";
    conditions[0].isNULL = true;
    conditions[0].isNotNULL = false;
    ASSERT_EQ(qmm.Select(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions), OK_RC);
    ASSERT_EQ(qmm.Update(relName, nColumns, columnNames, rhsValue, nConditions, conditions), OK_RC);
    ASSERT_EQ(qmm.Select(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions), OK_RC);
    conditions[0].isNULL = false;
    conditions[0].isNotNULL = true;
    ASSERT_EQ(qmm.Select(nSelAttrs, selAttrs, nRelations, relations, nConditions, conditions), OK_RC);
}