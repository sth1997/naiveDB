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
#include "printer.h"

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
    sm_mgr = &smm;
    ix_mgr = &ixm;
    rm_mgr = &rmm;
}

//
// QL_Manager::~QL_Manager()
//
// Destructor for the QL Manager
//
QL_Manager::~QL_Manager()
{
}

RC QL_Manager::CheckAttr(RelAttr attr, map<string, set<string>>& attr2rels, map<string, int>& relsCnt) {
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

template<typename T>
bool QL_Manager::matchValue(CompOp op, T lValue, T rValue) {
    switch(op) {
        case NO_OP:
            return true;
        case EQ_OP:
            return lValue == rValue;
        case LT_OP:
            return lValue < rValue;
        case GT_OP:
            return lValue > rValue;
        case LE_OP:
            return lValue <= rValue;
        case GE_OP:
            return lValue >= rValue;
        case NE_OP:
            return lValue != rValue;
        default:
            break;
    }
    return false;
}

//
// Handle the select clause
//
// 1. check database is opened
// 2. check relation is existed and not duplicated
// 3. check attrs existed and non-ambiguous
// 4. check conditions
RC QL_Manager::Select(int nSelAttrs, const RelAttr selAttrs[],
                      int nRelations, const char * const relations[],
                      int nConditions, const Condition conditions[])
{

    bool printPara = true;
    if (printPara) {

        int i;

        cout << "Select\n";

        cout << "   nSelAttrs = " << nSelAttrs << "\n";
        for (i = 0; i < nSelAttrs; i++)
            cout << "   selAttrs[" << i << "]:" << selAttrs[i] << "\n";

        cout << "   nRelations = " << nRelations << "\n";
        for (i = 0; i < nRelations; i++)
            cout << "   relations[" << i << "]:" << relations[i] << "\n";

        cout << "   nCondtions = " << nConditions << "\n";
        for (i = 0; i < nConditions; i++)
            cout << "   conditions[" << i << "]:" << conditions[i] << "\n";
    }

    RC rc;
    // 1. check database is opened
    if (!sm_mgr->DBOpened()) {
        return QL_DATABASE_CLOSED;
    }

    // 2. check relation is existed and not duplicated
    if (nRelations > 2) {
        return QL_NOT_SUPPORT_MULTI_JOIN_NOW;
    }
    vector<string> relnames;
    map<string, int> relsCnt;
    vector<DataRelInfo> dataRelInfos;
    vector<vector<DataAttrInfo>> attributes;
    int allAttrCnt = 0;
    map<string, DataAttrInfo> attr2info;
    map<string, set<string>> attr2rels;

    // relname in string
    for (int i = 0; i < nRelations; i++) {
        relnames.push_back(string(relations[i]));
    }
    // rel must be found in catalog
    for (int i = 0; i < nRelations; i++) {
        string relname(relations[i]);
        relsCnt[relname]++;
        dataRelInfos.push_back(DataRelInfo());
        RID rid;
        bool found;
        CHECK_NOZERO(sm_mgr->FindRel(relations[i], dataRelInfos[i], rid, found));
        // must found
        if (!found) {
            return QL_RELATION_NOT_FOUND;
        }
        vector<DataAttrInfo> tmp;
        CHECK_NOZERO(sm_mgr->FindAllAttrs(relations[i], tmp));
        attributes.push_back(tmp);
        // construct attr2rels
        for (auto attr : attributes[i]) {
            string attrname(attr.attrName);
            attr2info[relname+"."+attrname]=attr;
            attr2rels[attrname].insert(relname);
            allAttrCnt++;
        }
    } 
    // rel must not duplicated
    for (auto r : relsCnt) {
        if (r.second > 1) {
            return QL_RELATION_DUPLICATED;
        }
    }

    // 3. check attrs existed and non-ambiguous
    vector<RelAttr> changedSelAttrs;
    if (nSelAttrs == 1 && strcmp(selAttrs[0].attrName, "*") == 0) {
        nSelAttrs = 0;
        for (int i = 0; i < nRelations; i++) {
            for (auto attr : attributes[i]) {
                changedSelAttrs.push_back(RelAttr());
                changedSelAttrs[nSelAttrs].relName = strdup(relations[i]);
                changedSelAttrs[nSelAttrs].attrName = strdup(attr.attrName);
                nSelAttrs++;
            }
        }
    } else {
        for (int i = 0; i < nSelAttrs; i++) {
            if ((rc = CheckAttr(selAttrs[i], attr2rels, relsCnt))) {
                return rc;
            }
            changedSelAttrs.push_back(RelAttr());
            changedSelAttrs[i].attrName = strdup(selAttrs[i].attrName);
            if (selAttrs[i].relName == NULL) {
                string attrname(selAttrs[i].attrName);
                string relname = *(attr2rels[attrname].begin());
                changedSelAttrs[i].relName = strdup(relname.c_str());
            }
            else
            {
                changedSelAttrs[i].relName = strdup(selAttrs[i].relName);
            }
        }
    }
    vector<DataAttrInfo> changedSelAttrInfo;
    for (int i = 0; i < nSelAttrs; i++) {
        changedSelAttrInfo.push_back(attr2info[string(changedSelAttrs[i].relName)+"."+string(changedSelAttrs[i].attrName)]);
    }

    // 4. check conditions
    for (int i = 0; i < nConditions; i++) {
        if (conditions[i].isNULL || conditions[i].isNotNULL) {
            continue;
        }
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

    bool optimize = false;
    if (!optimize) {
        vector<char*> res[2];
        // scan every file
        for (int i = 0; i < nRelations; i++) {
            string relname(relations[i]);
            RM_FileHandle rm_fhdl;
            rm_mgr->OpenFile(relations[i], rm_fhdl);
            RM_FileScan rm_fscan;
            int a = 0;
            rm_fscan.OpenScan(rm_fhdl, INT, 4, 0, NO_OP, &a);
            RM_Record rec;
            char* pData;
            // scan every record
            while (rm_fscan.GetNextRec(rec) == OK_RC) {
                rec.GetData(pData);
                bool matched = true;

                // init bitmap
                int numBytes = (dataRelInfos[i].attrCount + 7) / 8;
                DataAttrInfo last = attributes[i][attributes[i].size() - 1];
                char* c = pData + last.offset + last.attrLength;
                RM_BitMap bitmap(numBytes, c);

                // check every conditions
                for (int j = 0; j < nConditions; j++) {
                    // skip if not this rel
                    string lrname(conditions[j].lhsAttr.relName);
                    string laname(conditions[j].lhsAttr.attrName);
                    DataAttrInfo linfo = attr2info[lrname+"."+laname];

                    if (lrname != relname) {
                        continue;
                    }

                    int pos;
                    DataAttrInfo tmp;
                    bool tmpf;
                    RID rid;
                    sm_mgr->FindAttr(conditions[j].lhsAttr.relName, conditions[j].lhsAttr.attrName, tmp, rid, tmpf, &pos);
                    bool isNull = false;
                    bitmap.isFree(pos, isNull);
                    if (!conditions[j].isNULL && !conditions[j].isNotNULL) {
                        if (isNull) {
                            matched = false;
                        } else {
                            if (conditions[j].bRhsIsAttr) {
                                string rrname(conditions[j].rhsAttr.relName);
                                if (rrname != relname) {
                                    continue;
                                }
                            }
                            switch(linfo.attrType) {
                                case INT: {
                                    int lvalue;
                                    memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                                    int rvalue;
                                    if (conditions[j].bRhsIsAttr) {
                                        string rrname(conditions[j].rhsAttr.relName);
                                        string raname(conditions[j].rhsAttr.attrName);
                                        DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                                        memcpy(&rvalue, pData+rinfo.offset, rinfo.attrLength);
                                    } else {
                                        memcpy(&rvalue, conditions[j].rhsValue.data, 4);
                                    }
                                    matched = matchValue(conditions[j].op, lvalue, rvalue);
                                    break;
                                }
                                case FLOAT: {
                                    float lvalue;
                                    memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                                    float rvalue;
                                    if (conditions[j].bRhsIsAttr) {
                                        string rrname(conditions[j].rhsAttr.relName);
                                        string raname(conditions[j].rhsAttr.attrName);
                                        DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                                        memcpy(&rvalue, pData+rinfo.offset, rinfo.attrLength);
                                    } else {
                                        memcpy(&rvalue, conditions[j].rhsValue.data, 4);
                                    }
                                    matched = matchValue(conditions[j].op, lvalue, rvalue);
                                    break;
                                }
                                case STRING: {
                                    string lvalue;
                                    memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                                    string rvalue;
                                    if (conditions[j].bRhsIsAttr) {
                                        string rrname(conditions[j].rhsAttr.relName);
                                        string raname(conditions[j].rhsAttr.attrName);
                                        DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                                        memcpy(&rvalue, pData+rinfo.offset, rinfo.attrLength);
                                    } else {
                                        memcpy(&rvalue, conditions[j].rhsValue.data, linfo.attrLength);
                                    }
                                    matched = matchValue(conditions[j].op, lvalue, rvalue);
                                    break;
                                }
                                default: {
                                    ;
                                }
                            }
                        }
                    } else {
                        if (conditions[j].isNULL) {
                            matched = isNull ? true : false;
                        } else {
                            matched = isNull ? false : true;
                        }
                    }
                    if (!matched) {
                        break;
                    }
                }
                if (matched) {
                    char* tmp = new char[rec.GetRecordSize()];
                    memcpy(tmp, pData, rec.GetRecordSize());
                    res[i].push_back(tmp);
                }
            }
            rm_fscan.CloseScan();
        }
        if (nRelations == 1) {
            Printer p(*sm_mgr, changedSelAttrInfo, changedSelAttrInfo.size(), attributes);
            p.PrintHeader(cout);
            for (auto r : res[0]) {
                p.Print(cout, r);
            }
            p.PrintFooter(cout);
        }
        // cross-products
        if (nRelations == 2) {
            Printer p(*sm_mgr, changedSelAttrInfo, changedSelAttrInfo.size(), attributes);
            p.PrintHeader(cout);
            char* pData = new char[dataRelInfos[0].recordSize + dataRelInfos[1].recordSize];
            for (unsigned i = 0; i < res[0].size(); i++) {
                memcpy(pData, res[0][i], dataRelInfos[0].recordSize);
                for (unsigned j = 0; j < res[1].size(); j++) {
                    bool matched = true;
                    for (int k = 0; k < nConditions; k++) {
                        if (conditions[k].bRhsIsAttr) {
                            string lrname(conditions[k].lhsAttr.relName);
                            string rrname(conditions[k].rhsAttr.relName);
                            if (lrname != rrname) {
                                string laname(conditions[k].lhsAttr.attrName);
                                DataAttrInfo linfo = attr2info[lrname+"."+laname];
                                string raname(conditions[k].rhsAttr.attrName);
                                DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                                // swap if out of order
                                if (lrname != relnames[0]) {
                                    DataAttrInfo tmp = linfo;
                                    linfo = rinfo;
                                    rinfo = tmp;
                                }
                                switch (linfo.attrType) {
                                    case INT: {
                                        int lvalue;
                                        memcpy(&lvalue, res[0][i]+linfo.offset, linfo.attrLength);
                                        int rvalue;
                                        memcpy(&rvalue, res[1][i]+rinfo.offset, linfo.attrLength);
                                        matched = matchValue(conditions[k].op, lvalue, rvalue);
                                        break;
                                    }
                                    case FLOAT: {
                                        float lvalue;
                                        memcpy(&lvalue, res[0][i]+linfo.offset, linfo.attrLength);
                                        float rvalue;
                                        memcpy(&rvalue, res[1][i]+rinfo.offset, rinfo.attrLength);
                                        matched = matchValue(conditions[k].op, lvalue, rvalue);
                                        break;
                                    }
                                    case STRING: {
                                        string lvalue;
                                        memcpy(&lvalue, res[0][i]+linfo.offset, linfo.attrLength);
                                        string rvalue;
                                        memcpy(&rvalue, res[1][i]+rinfo.offset, rinfo.attrLength);
                                        matched = matchValue(conditions[k].op, lvalue, rvalue);
                                        break;
                                    }
                                    default: {
                                        ;
                                    }
                                }
                            }
                        }
                    }
                    memcpy(pData + dataRelInfos[0].recordSize, res[1][j], dataRelInfos[1].recordSize);
                    if (matched) {
                        p.Print(cout, pData);
                    }
                }
            }
            delete []pData;
            p.PrintFooter(cout);
        }
        for (int i = 0; i < 2; ++i)
            for (auto x : res[i])
                delete []x;
    } else {
        // index scan
    }

    for (auto x : changedSelAttrs)
    {
        free(x.relName);
        free(x.attrName);
    }
    return OK_RC;
}

//
// Insert the values into relName
//
RC QL_Manager::Insert(const char *relName,
                      int nValues, const Value values[])
{

    bool printPara = true;
    if (printPara) {
        int i;

        cout << "Insert\n";

        cout << "   relName = " << relName << "\n";
        cout << "   nValues = " << nValues << "\n";
        for (i = 0; i < nValues; i++)
            cout << "   values[" << i << "]:" << values[i] << "\n";
    }

    // get data rel info from rel
    RID rid;
    bool found = false;
    DataRelInfo dataRelInfo;
    sm_mgr->FindRel(relName, dataRelInfo, rid, found);
    if (!found) {
        return QL_RELATION_NOT_FOUND;
    }

    // get attr length from rel
    vector<DataAttrInfo> dataAttrInfos;
    sm_mgr->FindAllAttrs(relName, dataAttrInfos);
    DataAttrInfo last = dataAttrInfos[dataAttrInfos.size() - 1];

    // insert into rm file
    RM_FileHandle rm_fhdl;
    rm_mgr->OpenFile(relName, rm_fhdl);
    char* recData = new char[dataRelInfo.recordSize];
    memset(recData, 0, dataRelInfo.recordSize);
    int numBytes = (dataRelInfo.attrCount + 7) / 8;
    char* c = recData + last.offset + last.attrLength;
    RM_BitMap bitmap(numBytes, c);
    for (int i = 0; i < nValues; i++) {
        if (values[i].type == NULLTYPE) {
            if (!dataAttrInfos[i].couldBeNULL) {
                return QL_ATTR_CANT_BE_NULL;
            }
            bitmap.set(i, true);
        } else {
            switch(dataAttrInfos[i].attrType) {
                case INT: {
                    int value = *static_cast<int*>(values[i].data);
                    // cout << "insert " << value << " to " << dataAttrInfos[i].offset << " with " << dataAttrInfos[i].attrLength <<  endl;
                    memcpy(recData+dataAttrInfos[i].offset, &value, dataAttrInfos[i].attrLength);
                    break;
                }
                case FLOAT: {
                    float value = *static_cast<float*>(values[i].data);
                    memcpy(recData+dataAttrInfos[i].offset, &value, dataAttrInfos[i].attrLength);
                    break;
                }
                case STRING: {
                    char value[dataAttrInfos[i].attrLength];
                    char* valueChar = static_cast<char*>(values[i].data);
                    strcpy(value, valueChar);
                    memcpy(recData+dataAttrInfos[i].offset, valueChar, dataAttrInfos[i].attrLength);
                }
                default: {
                    ;
                }
            }
            bitmap.set(i, false);
        }
    }
    RC rc;
    CHECK_NOZERO(rm_fhdl.InsertRec(recData, rid));
    rm_mgr->CloseFile(rm_fhdl);

    bool printResult = false;
    if (printResult) {
        vector<vector<DataAttrInfo> > allAdataAttrInfos;
        allAdataAttrInfos.push_back(dataAttrInfos);
        cout << "Insert tuple:" << endl;
        Printer p(*sm_mgr, dataAttrInfos, dataAttrInfos.size(), allAdataAttrInfos);
        p.PrintHeader(cout);
        p.Print(cout, recData);
        p.PrintFooter(cout);
    }
    delete []recData;

    // TODO
    // insert into index

    return OK_RC;
}

//
// Delete from the relName all tuples that satisfy conditions
//
RC QL_Manager::Delete(const char *relName,
                      int nConditions, const Condition conditions[])
{
    RC rc;
    RID rid;
    vector<DataAttrInfo> attributes;
    map<string, DataAttrInfo> attr2info;
    CHECK_NOZERO(sm_mgr->FindAllAttrs(relName, attributes));
    for (auto attr : attributes) {
        attr2info[string(attr.relName)+"."+string(attr.attrName)] = attr;
    }
    // check each condition
    for (int i = 0; i < nConditions; i++) {
        bool found;
        DataAttrInfo dataAttrInfo;
        CHECK_NOZERO(sm_mgr->FindAttr(conditions[i].lhsAttr.relName, conditions[i].lhsAttr.attrName, dataAttrInfo, rid, found));
        AttrType lhsType = dataAttrInfo.attrType;
        AttrType rhsType;
        if (conditions[i].bRhsIsAttr) {
            CHECK_NOZERO(sm_mgr->FindAttr(conditions[i].rhsAttr.relName, conditions[i].rhsAttr.attrName, dataAttrInfo, rid, found));
            rhsType = dataAttrInfo.attrType;
        } else {
            rhsType = conditions[i].rhsValue.type;
        }
        if (lhsType != rhsType) {
            return QL_INCOMPATIBLE_TYPE;
        }
    }
    bool optimize = false;
    if (!optimize) {
        RM_FileHandle rm_fhdl;
        rm_mgr->OpenFile(relName, rm_fhdl);
        RM_FileScan rm_fscan;
        int a = 0;
        rm_fscan.OpenScan(rm_fhdl, INT, 4, 0, NO_OP, &a);
        RM_Record rec;
        char* pData;
        // scan every record
        while (rm_fscan.GetNextRec(rec) == OK_RC) {
            rec.GetData(pData);
            bool matched = true;
            for (int i = 0; i < nConditions; i++) {
                string lrname(conditions[i].lhsAttr.relName);
                string laname(conditions[i].lhsAttr.attrName);
                DataAttrInfo linfo = attr2info[lrname+"."+laname];
                switch(linfo.attrType) {
                    case INT: {
                        int lvalue;
                        memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                        int rvalue;
                        if (conditions[i].bRhsIsAttr) {
                            string rrname(conditions[i].rhsAttr.relName);
                            string raname(conditions[i].rhsAttr.attrName);
                            DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                            memcpy(&rvalue, pData+rinfo.offset, rinfo.attrLength);
                        } else {
                            memcpy(&rvalue, conditions[i].rhsValue.data, 4);
                        }
                        matched = matchValue(conditions[i].op, lvalue, rvalue);
                        break;
                    }
                    case FLOAT: {
                        float lvalue;
                        memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                        float rvalue;
                        if (conditions[i].bRhsIsAttr) {
                            string rrname(conditions[i].rhsAttr.relName);
                            string raname(conditions[i].rhsAttr.attrName);
                            DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                            memcpy(&rvalue, pData+rinfo.offset, rinfo.attrLength);
                        } else {
                            memcpy(&rvalue, conditions[i].rhsValue.data, 4);
                        }
                        matched = matchValue(conditions[i].op, lvalue, rvalue);
                        break;
                    }
                    case STRING: {
                        string lvalue;
                        memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                        string rvalue;
                        if (conditions[i].bRhsIsAttr) {
                            string rrname(conditions[i].rhsAttr.relName);
                            string raname(conditions[i].rhsAttr.attrName);
                            DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                            memcpy(&rvalue, pData+rinfo.offset, rinfo.attrLength);
                        } else {
                            memcpy(&rvalue, conditions[i].rhsValue.data, linfo.attrLength);
                        }
                        matched = matchValue(conditions[i].op, lvalue, rvalue);
                        break;
                    }
                    default: {
                        ;
                    }
                }
                if (!matched) {
                    break;
                }
            }
            if (matched) {
                rec.GetRid(rid);
                rm_fhdl.DeleteRec(rid);
            }
        }
        rm_fscan.CloseScan();
        rm_mgr->CloseFile(rm_fhdl);
    } else {

    }
    bool printPara = true;
    if (printPara) {
        int i;

        cout << "Delete\n";

        cout << "   relName = " << relName << "\n";
        cout << "   nCondtions = " << nConditions << "\n";
        for (i = 0; i < nConditions; i++)
            cout << "   conditions[" << i << "]:" << conditions[i] << "\n";
    }
    return OK_RC;
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
    RC rc;
    vector<DataAttrInfo> attributes;
    map<string, DataAttrInfo> attr2info;
    CHECK_NOZERO(sm_mgr->FindAllAttrs(relName, attributes));
    for (auto attr : attributes) {
        attr2info[string(attr.relName)+"."+string(attr.attrName)] = attr;
    }
    // check attr
    DataAttrInfo updAttrInfo = attr2info[string(updAttr.relName)+"."+string(updAttr.attrName)];
    if (bIsValue) {
        if (rhsValue.type != updAttrInfo.attrType) {
            return QL_INCOMPATIBLE_TYPE;
        }
    } else {
        DataAttrInfo rhsRelAttrInfo = attr2info[string(rhsRelAttr.relName)+"."+string(rhsRelAttr.attrName)];
        if (rhsRelAttrInfo.attrType != updAttrInfo.attrType) {
            return QL_INCOMPATIBLE_TYPE;
        }
    }
    // check each condition
    for (int i = 0; i < nConditions; i++) {
        bool found;
        RID rid;
        DataAttrInfo dataAttrInfo;
        CHECK_NOZERO(sm_mgr->FindAttr(conditions[i].lhsAttr.relName, conditions[i].lhsAttr.attrName, dataAttrInfo, rid, found));
        AttrType lhsType = dataAttrInfo.attrType;
        AttrType rhsType;
        if (conditions[i].bRhsIsAttr) {
            CHECK_NOZERO(sm_mgr->FindAttr(conditions[i].rhsAttr.relName, conditions[i].rhsAttr.attrName, dataAttrInfo, rid, found));
            rhsType = dataAttrInfo.attrType;
        } else {
            rhsType = conditions[i].rhsValue.type;
        }
        if (lhsType != rhsType) {
            return QL_INCOMPATIBLE_TYPE;
        }
    }
    bool optimize = false;
    if (!optimize) {
        RM_FileHandle rm_fhdl;
        rm_mgr->OpenFile(relName, rm_fhdl);
        RM_FileScan rm_fscan;
        int a = 0;
        rm_fscan.OpenScan(rm_fhdl, INT, 4, 0, NO_OP, &a);
        RM_Record rec;
        char* pData;
        // scan every record
        while (rm_fscan.GetNextRec(rec) == OK_RC) {
            rec.GetData(pData);
            bool matched = true;
            for (int i = 0; i < nConditions; i++) {
                string lrname(conditions[i].lhsAttr.relName);
                string laname(conditions[i].lhsAttr.attrName);
                DataAttrInfo linfo = attr2info[lrname+"."+laname];
                switch(linfo.attrType) {
                    case INT: {
                        int lvalue;
                        memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                        int rvalue;
                        if (conditions[i].bRhsIsAttr) {
                            string rrname(conditions[i].rhsAttr.relName);
                            string raname(conditions[i].rhsAttr.attrName);
                            DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                            memcpy(&rvalue, pData+rinfo.offset, rinfo.attrLength);
                        } else {
                            memcpy(&rvalue, conditions[i].rhsValue.data, 4);
                        }
                        matched = matchValue(conditions[i].op, lvalue, rvalue);
                        break;
                    }
                    case FLOAT: {
                        float lvalue;
                        memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                        float rvalue;
                        if (conditions[i].bRhsIsAttr) {
                            string rrname(conditions[i].rhsAttr.relName);
                            string raname(conditions[i].rhsAttr.attrName);
                            DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                            memcpy(&rvalue, pData+rinfo.offset, rinfo.attrLength);
                        } else {
                            memcpy(&rvalue, conditions[i].rhsValue.data, 4);
                        }
                        matched = matchValue(conditions[i].op, lvalue, rvalue);
                        break;
                    }
                    case STRING: {
                        string lvalue;
                        memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                        string rvalue;
                        if (conditions[i].bRhsIsAttr) {
                            string rrname(conditions[i].rhsAttr.relName);
                            string raname(conditions[i].rhsAttr.attrName);
                            DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                            memcpy(&rvalue, pData+rinfo.offset, rinfo.attrLength);
                        } else {
                            memcpy(&rvalue, conditions[i].rhsValue.data, linfo.attrLength);
                        }
                        matched = matchValue(conditions[i].op, lvalue, rvalue);
                        break;
                    }
                    default: {
                        ;
                    }
                }
                if (!matched) {
                    break;
                }
            }
            if (matched) {
                char* tmp;
                rec.GetData(tmp);
                string lrname(updAttr.relName);
                string laname(updAttr.attrName);
                DataAttrInfo linfo = attr2info[lrname+"."+laname];
                if (bIsValue) {
                    memcpy(tmp+linfo.offset, rhsValue.data, linfo.attrLength);
                } else {
                    string rrname(rhsRelAttr.relName);
                    string raname(rhsRelAttr.attrName);
                    DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                    memcpy(tmp+linfo.offset, tmp+rinfo.offset, rinfo.attrLength);
                }
                rm_fhdl.UpdateRec(rec);
            }
        }
        rm_fscan.CloseScan();
        rm_mgr->CloseFile(rm_fhdl);
    } else {

    }
    bool printPara = true;
    if (printPara) {
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
    }
    return OK_RC;
}

//
// Update from the relName all tuples that satisfy conditions
//
RC QL_Manager::Update(const char *relName,
                      int nColumns,
                      const char* const columnNames[],
                      const Value values[],
                      int nConditions, const Condition conditions[])
{
    RC rc;
    vector<DataAttrInfo> attributes;
    map<string, DataAttrInfo> attr2info;
    CHECK_NOZERO(sm_mgr->FindAllAttrs(relName, attributes));
    for (auto attr : attributes) {
        attr2info[string(attr.relName)+"."+string(attr.attrName)] = attr;
    }
    // check each condition
    for (int i = 0; i < nConditions; i++) {
        bool found;
        RID rid;
        DataAttrInfo dataAttrInfo;
        CHECK_NOZERO(sm_mgr->FindAttr(conditions[i].lhsAttr.relName, conditions[i].lhsAttr.attrName, dataAttrInfo, rid, found));
        AttrType lhsType = dataAttrInfo.attrType;
        AttrType rhsType;
        if (conditions[i].bRhsIsAttr) {
            CHECK_NOZERO(sm_mgr->FindAttr(conditions[i].rhsAttr.relName, conditions[i].rhsAttr.attrName, dataAttrInfo, rid, found));
            rhsType = dataAttrInfo.attrType;
        } else {
            rhsType = conditions[i].rhsValue.type;
        }
        if (lhsType != rhsType) {
            return QL_INCOMPATIBLE_TYPE;
        }
    }
    bool optimize = false;
    if (!optimize) {
        RM_FileHandle rm_fhdl;
        rm_mgr->OpenFile(relName, rm_fhdl);
        RM_FileScan rm_fscan;
        int a = 0;
        rm_fscan.OpenScan(rm_fhdl, INT, 4, 0, NO_OP, &a);
        RM_Record rec;
        char* pData;
        // scan every record
        while (rm_fscan.GetNextRec(rec) == OK_RC) {
            rec.GetData(pData);
            bool matched = true;
            for (int i = 0; i < nConditions; i++) {
                string lrname(conditions[i].lhsAttr.relName);
                string laname(conditions[i].lhsAttr.attrName);
                DataAttrInfo linfo = attr2info[lrname+"."+laname];
                switch(linfo.attrType) {
                    case INT: {
                        int lvalue;
                        memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                        int rvalue;
                        if (conditions[i].bRhsIsAttr) {
                            string rrname(conditions[i].rhsAttr.relName);
                            string raname(conditions[i].rhsAttr.attrName);
                            DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                            memcpy(&rvalue, pData+rinfo.offset, rinfo.attrLength);
                        } else {
                            memcpy(&rvalue, conditions[i].rhsValue.data, 4);
                        }
                        matched = matchValue(conditions[i].op, lvalue, rvalue);
                        break;
                    }
                    case FLOAT: {
                        float lvalue;
                        memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                        float rvalue;
                        if (conditions[i].bRhsIsAttr) {
                            string rrname(conditions[i].rhsAttr.relName);
                            string raname(conditions[i].rhsAttr.attrName);
                            DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                            memcpy(&rvalue, pData+rinfo.offset, rinfo.attrLength);
                        } else {
                            memcpy(&rvalue, conditions[i].rhsValue.data, 4);
                        }
                        matched = matchValue(conditions[i].op, lvalue, rvalue);
                        break;
                    }
                    case STRING: {
                        string lvalue;
                        memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                        string rvalue;
                        if (conditions[i].bRhsIsAttr) {
                            string rrname(conditions[i].rhsAttr.relName);
                            string raname(conditions[i].rhsAttr.attrName);
                            DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                            memcpy(&rvalue, pData+rinfo.offset, rinfo.attrLength);
                        } else {
                            memcpy(&rvalue, conditions[i].rhsValue.data, linfo.attrLength);
                        }
                        matched = matchValue(conditions[i].op, lvalue, rvalue);
                        break;
                    }
                    default: {
                        ;
                    }
                }
                if (!matched) {
                    break;
                }
            }
            if (matched) {
                char* tmp;
                rec.GetData(tmp);
                for (int i = 0; i < nColumns; i++) {
                    string lrname(relName);
                    string laname(columnNames[i]);
                    DataAttrInfo linfo = attr2info[lrname+"."+laname];
                    memcpy(tmp+linfo.offset, values[i].data, linfo.attrLength);
                }
                rm_fhdl.UpdateRec(rec);
            }
        }
        rm_fscan.CloseScan();
        rm_mgr->CloseFile(rm_fhdl);
    } else {

    }
    bool printPara = true;
    if (printPara) {
        int i;

        cout << "Update\n";

        cout << "   relName = " << relName << "\n";
        for (i = 0; i < nColumns; i++) {
            cout << "   columnNames:[" << i << "]: " << columnNames[i] << "\n";
        }
        for (i = 0; i < nColumns; i++) {
            cout << "   values:[" << i << "]: " << values[i] << "\n";
        }
        cout << "   nCondtions = " << nConditions << "\n";
        for (i = 0; i < nConditions; i++)
            cout << "   conditions[" << i << "]:" << conditions[i] << "\n";
    }
    return OK_RC;
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
