//
// ql_manager_stub.cc
//

// Note that for the SM component (HW3) the QL is actually a
// simple stub that will allow everything to compile.  Without
// a QL stub, we would need two parsers.

#include <cstdio>
#include <iostream>
#include <sys/times.h>
#include <sys/types.h>
#include <cassert>
#include <unistd.h>
#include <map>
#include <set>
#include <string>
#include "naivedb.h"
#include "ql.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"

using namespace std;

#define CHECK_NOZERO(x)            \
    {                              \
        if ((rc = x)) {            \
            printf("ql warning; %d\n", rc);    \
            if (rc < 0) {          \
                return QL_SMERROR; \
            }                      \
        }                          \
    }

//
// QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm)
//
// Constructor for the QL Manager
//
QL_Manager::QL_Manager(SM_Manager &smm, IX_Manager &ixm, RM_Manager &rmm)
{
    // Can't stand unused variable warnings!
    assert (&smm && &ixm && &rmm);
}

//
// QL_Manager::~QL_Manager()
//
// Destructor for the QL Manager
//
QL_Manager::~QL_Manager()
{
}

RC CheckAttr(RelAttr attr, map<string, set<string>>& attr2rels, map<string, int>& relsCnt) {
    string attrname(attr.attrName);
    if (attr.relName == NULL) {
        // attr must belong to someone
        if (attr2rels[attrname].size() == 0) {
            return QL_ATTR_NOT_FOUND;
        }
        // attr must belong to only one
        if (attr2rels[attrname].size() > 1) {
            return QL_ATTR_AMBIGUOUS;
        }
    } else {
        string relname(attr.relName);
        // rel must in from clause
        if (relsCnt[relname] == 0) {
            return QL_SELECT_REL_NOT_IN_FROM;
        }
        // rel must have this attr
        if (attr2rels[attrname].find(relname) == attr2rels[attrname].end()) {
            return QL_SELECT_REL_DIDNT_HAVE_THIS_ATTR;
        }
    }
    return OK_RC;
}

//
// Handle the select clause
//
// 1. check database is opened
// 2. check relation is existed and not duplicated
// 3. check attrs existed and non-ambiguous
// 4. check conditions
// 5. make iterator
RC QL_Manager::Select(int nSelAttrs, const RelAttr selAttrs[],
                      int nRelations, const char * const relations[],
                      int nConditions, const Condition conditions[])
{
    RC rc;
    // 1. check database is opened
    if (!sm_mgr->DBOpened()) {
        return QL_DATABASE_CLOSED;
    }

    // 2. check relation is existed and not duplicated
    vector<DataRelInfo> dataRelInfos;
    vector<DataAttrInfo> attributes[nRelations];
    map<string, int> relsCnt;
    map<string, set<string>> attr2rels;
    // rel must be found in catalog
    for (int i = 0; i < nRelations; i++) {
        string s(relations[i]);
        relsCnt[s]++;
        dataRelInfos.push_back(DataRelInfo());
        RID rid;
        bool found;
        CHECK_NOZERO(sm_mgr->FindRel(relations[i], dataRelInfos[i], rid, found));
        // must found
        if (!found) {
            return QL_RELATION_NOT_FOUND;
        }
        CHECK_NOZERO(sm_mgr->FindAllAttrs(relations[i], attributes[i]));
        // construct attr2rels
        for (auto attr : attributes[i]) {
            string attrname(attr.attrName);
            attr2rels[attrname].insert(s);
        }
    } 
    // rel must not duplicated
    for (auto r : relsCnt) {
        if (r.second > 1) {
            return QL_RELATION_DUPLICATED;
        }
    }

    // 3. check attrs existed and non-ambiguous
    vector<RelAttr> changedSelAttr;
    if (nSelAttrs == 1 && strcmp(selAttrs[0].attrName, "*") == 0) {
        nSelAttrs = 0;
        for (int i = 0; i < nRelations; i++) {
            for (auto attr : attributes[i]) {
                changedSelAttr.push_back(RelAttr());
                changedSelAttr[nSelAttrs].relName = attr.relName;
                changedSelAttr[nSelAttrs].attrName = attr.attrName;
                nSelAttrs++;
            }
        }
    } else {
        for (int i = 0; i < nSelAttrs; i++) {
            if ((rc = CheckAttr(selAttrs[i], attr2rels, relsCnt))) {
                return rc;
            }
            changedSelAttr.push_back(RelAttr());
            changedSelAttr[i].relName = selAttrs[i].relName;
            changedSelAttr[i].attrName = selAttrs[i].attrName;
        }

    }

    // 4. check conditions
    for (int i = 0; i < nConditions; i++) {
        // check each condition
        // 4.1 check lhsAttr
        if ((rc = CheckAttr(conditions[i].lhsAttr, attr2rels, relsCnt))) {
            return rc;
        }
        RID rid;
        bool found;
        DataAttrInfo dataAttrInfo;
        CHECK_NOZERO(sm_mgr->FindAttr(conditions[i].lhsAttr.relName, conditions[i].lhsAttr.attrName, dataAttrInfo, rid, found));
        AttrType lhsType = dataAttrInfo.attrType;
        AttrType rhsType;
        // 4.2 check rhsAttr && 4.3 check compatibility
        if (conditions[i].bRhsIsAttr) {
            if ((rc = CheckAttr(conditions[i].rhsAttr, attr2rels, relsCnt))) {
                return rc;
            }
            CHECK_NOZERO(sm_mgr->FindAttr(conditions[i].rhsAttr.relName, conditions[i].rhsAttr.attrName, dataAttrInfo, rid, found));
            rhsType = dataAttrInfo.attrType;
        } else {
            rhsType = conditions[i].rhsValue.type;
        }
        if (lhsType != rhsType) {
            return QL_INCOMPATIBLE_TYPE;
        }
    }

    if (false) {

        int i;

        cout << "Select\n";

        cout << "   nSelAttrs = " << nSelAttrs << "\n";
        for (i = 0; i < nSelAttrs; i++)
            cout << "   selAttrs[" << i << "]:" << selAttrs[i] << "\n";

        cout << "   nRelations = " << nRelations << "\n";
        for (i = 0; i < nRelations; i++)
            cout << "   relations[" << i << "] " << relations[i] << "\n";

        cout << "   nCondtions = " << nConditions << "\n";
        for (i = 0; i < nConditions; i++)
            cout << "   conditions[" << i << "]:" << conditions[i] << "\n";
    }

    return 0;
}

//
// Insert the values into relName
//
RC QL_Manager::Insert(const char *relName,
                      int nValues, const Value values[])
{
    int i;

    cout << "Insert\n";

    cout << "   relName = " << relName << "\n";
    cout << "   nValues = " << nValues << "\n";
    for (i = 0; i < nValues; i++)
        cout << "   values[" << i << "]:" << values[i] << "\n";

    return 0;
}

//
// Delete from the relName all tuples that satisfy conditions
//
RC QL_Manager::Delete(const char *relName,
                      int nConditions, const Condition conditions[])
{
    int i;

    cout << "Delete\n";

    cout << "   relName = " << relName << "\n";
    cout << "   nCondtions = " << nConditions << "\n";
    for (i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

    return 0;
}


//
// Update from the relName all tuples that satisfy conditions
//
RC QL_Manager::Update(const char *relName,
                      const RelAttr &updAttr,
                      const int bIsValue,
                      const RelAttr &rhsRelAttr,
                      const Value &rhsValue,
                      int nConditions, const Condition conditions[])
{
    int i;

    cout << "Update\n";

    cout << "   relName = " << relName << "\n";
    cout << "   updAttr:" << updAttr << "\n";
    if (bIsValue)
        cout << "   rhs is value: " << rhsValue << "\n";
    else
        cout << "   rhs is attribute: " << rhsRelAttr << "\n";

    cout << "   nCondtions = " << nConditions << "\n";
    for (i = 0; i < nConditions; i++)
        cout << "   conditions[" << i << "]:" << conditions[i] << "\n";

    return 0;
}

//
// void QL_PrintError(RC rc)
//
// This function will accept an Error code and output the appropriate
// error.
//
void QL_PrintError(RC rc)
{
    cout << "QL_PrintError\n   rc=" << rc << "\n";
}
