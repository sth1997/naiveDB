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
#include "ql_internal.h"

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
    printPara = false;
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

bool matched(AttrType t1, AttrType& t2, char* data) {
    if (t1 == NULLTYPE || t2 == NULLTYPE) {
        return true;
    }
    switch(t1) {
        case INT: {
            return t2 == INT;
            break;
        }
        case FLOAT: {
            if (t2 == STRING) {
                return false;
            }
            if (t2 == INT) {
                float tmp = *(int*) data;
                t2 = FLOAT;
                memcpy(data, &tmp, 4);
            }
            return true;
            break;
        }
        case STRING: {
            return t2 == STRING;
        }
        default: {
            cout << "aaa" << endl;
        }
    }
    return false;
}

RC QL_Manager::RM_GetRecords(const char* const relation, int nConditions, const Condition conditions[], vector<RM_Record>& rm_records) {

    // find dataRelInfo
    RC rc;
    DataRelInfo dataRelInfo;
    RID rid;
    bool found;
    CHECK_NOZERO(sm_mgr->FindRel(relation, dataRelInfo, rid, found));

    // find dataAttrInfos, build map form name to dataAttrInfo
    vector<DataAttrInfo> dataAttrInfos;
    map<string, DataAttrInfo> attr2info;
    CHECK_NOZERO(sm_mgr->FindAllAttrs(relation, dataAttrInfos));
    for (auto attr : dataAttrInfos) {
        attr2info[string(attr.relName)+"."+string(attr.attrName)] = attr;
    }
    
    RM_FileHandle rm_fhdl;
    CHECK_NOZERO(rm_mgr->OpenFile(relation, rm_fhdl));
    RM_FileScan rm_fscan;
    int a = 0;
    CHECK_NOZERO(rm_fscan.OpenScan(rm_fhdl, INT, 4, 0, NO_OP, &a));
    RM_Record rec;
    char* pData;
    DataAttrInfo last = dataAttrInfos.back();
    int numBytes = (dataRelInfo.attrCount + 7) / 8;
    string relname(relation);
    // scan every record
    while (rm_fscan.GetNextRec(rec) == OK_RC) {
        CHECK_NOZERO(rec.GetData(pData));
        bool matched = true;

        // init bitmap
        char* c = pData + last.offset + last.attrLength;
        RM_BitMap bitmap(numBytes, c);

        
        // check every conditions
        for (int i = 0; i < nConditions; i++) {
            string laname(conditions[i].lhsAttr.attrName);
            if (conditions[i].lhsAttr.relName == NULL) {
                // skip if not in this rel
                if (attr2info.find(relname+"."+laname) == attr2info.end()) {
                    continue;
                }
            } else {
                // skip if not this rel
                if (strcmp(relation, conditions[i].lhsAttr.relName) != 0) {
                    continue;
                }
            }

            int pos;
            DataAttrInfo linfo;
            bool tmpf;
            RID rid;
            // cout << relation << "." << conditions[i].lhsAttr.attrName << endl;
            CHECK_NOZERO(sm_mgr->FindAttr(relation, conditions[i].lhsAttr.attrName, linfo, rid, tmpf, &pos));
            bool isNull = false;
            // cout << "pos: " << pos << endl;
            CHECK_NOZERO(bitmap.isFree(pos, isNull));
            if (!conditions[i].isNULL && !conditions[i].isNotNULL) {
                if (isNull) {
                    matched = false;
                } else {
                    if (conditions[i].bRhsIsAttr) {
                        string raname(conditions[i].rhsAttr.attrName);
                        if (conditions[i].rhsAttr.relName == NULL) {
                            // skip if not in this rel
                            if (attr2info.find(relname+"."+raname) == attr2info.end()) {
                                continue;
                            }
                        } else {
                            // skip if not this rel
                            if (strcmp(relation, conditions[i].rhsAttr.relName) != 0) {
                                continue;
                            }
                        }
                    }
                    switch(linfo.attrType) {
                        case INT: {
                            int lvalue;
                            memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                            int rvalue;
                            if (conditions[i].bRhsIsAttr) {
                                string rrname(relation);
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
                                string rrname(relation);
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
                            string lvalue(pData + linfo.offset);
                            //memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                            string rvalue;
                            if (conditions[i].bRhsIsAttr) {
                                string rrname(relation);
                                string raname(conditions[i].rhsAttr.attrName);
                                DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                                //memcpy(&rvalue, pData+rinfo.offset, rinfo.attrLength);
                                string tmp(pData+rinfo.offset);
                                rvalue = tmp;
                            } else {
                                string tmp((char*)conditions[i].rhsValue.data);
                                rvalue = tmp;
                                //memcpy(&rvalue, conditions[i].rhsValue.data, linfo.attrLength);
                            }
                            matched = matchValue(conditions[i].op, lvalue, rvalue);
                            break;
                        }
                        default: {
                            ;
                        }
                    }
                }
            } else {
                if (conditions[i].isNULL) {
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
            rm_records.push_back(rec);
        }
    }
    CHECK_NOZERO(rm_fscan.CloseScan());
    CHECK_NOZERO(rm_mgr->CloseFile(rm_fhdl));
}

RC QL_Manager::IX_GetRecords(const char* const relation, int nConditions, const Condition conditions[], vector<RM_Record>& rm_records, int indexNo, int condNo) {
     // find dataRelInfo
    RC rc;
    DataRelInfo dataRelInfo;
    RID rid;
    bool found;
    CHECK_NOZERO(sm_mgr->FindRel(relation, dataRelInfo, rid, found));

    // find dataAttrInfos, build map form name to dataAttrInfo
    vector<DataAttrInfo> dataAttrInfos;
    map<string, DataAttrInfo> attr2info;
    CHECK_NOZERO(sm_mgr->FindAllAttrs(relation, dataAttrInfos));
    int primaryIndexNo = 0;

    for (auto attr : dataAttrInfos) {
        attr2info[string(attr.relName)+"."+string(attr.attrName)] = attr;
        if (attr.isPrimaryKey) {
            primaryIndexNo = attr.indexNo;
        }
    }
    
    RM_FileHandle rm_fhdl;
    IX_IndexHandle ix_ihdl;
    CHECK_NOZERO(rm_mgr->OpenFile(relation, rm_fhdl));
    CHECK_NOZERO(ix_mgr->OpenIndex(relation, indexNo, ix_ihdl));
    IX_IndexScan ix_scan;
    CHECK_NOZERO(ix_scan.OpenScan(ix_ihdl, conditions[condNo].op, conditions[condNo].rhsValue.data, NO_HINT));
    RM_Record rec;
    char* pData;
    string relname(relation);
    DataAttrInfo last = dataAttrInfos.back();
    int numBytes = (dataRelInfo.attrCount + 7) / 8;

    // scan every record
    while (ix_scan.GetNextEntry(rid) == OK_RC) {
        CHECK_NOZERO(rm_fhdl.GetRec(rid, rec));
        CHECK_NOZERO(rec.GetData(pData));
        bool matched = true;

        // init bitmap
        char* c = pData + last.offset + last.attrLength;
        RM_BitMap bitmap(numBytes, c);

        // check every conditions
        for (int i = 0; i < nConditions; i++) {
            string laname(conditions[i].lhsAttr.attrName);
            if (conditions[i].lhsAttr.relName == NULL) {
                // skip if not in this rel
                if (attr2info.find(relname+"."+laname) == attr2info.end()) {
                    continue;
                }
            } else {
                // skip if not this rel
                string lrname(conditions[i].lhsAttr.relName);
                if (lrname != relname) {
                    continue;
                }
            }

            int pos;
            DataAttrInfo linfo;
            bool tmpf;
            RID rid;
            CHECK_NOZERO(sm_mgr->FindAttr(relation, conditions[i].lhsAttr.attrName, linfo, rid, tmpf, &pos));
            bool isNull = false;
            CHECK_NOZERO(bitmap.isFree(pos, isNull));
            if (!conditions[i].isNULL && !conditions[i].isNotNULL) {
                if (isNull) {
                    matched = false;
                } else {
                    if (conditions[i].bRhsIsAttr) {
                        string raname(conditions[i].rhsAttr.attrName);
                        if (conditions[i].rhsAttr.relName == NULL) {
                            // skip if not in this rel
                            if (attr2info.find(relname+"."+raname) == attr2info.end()) {
                                continue;
                            }
                        } else {
                            // skip if not this rel
                            if (strcmp(relation, conditions[i].rhsAttr.relName) != 0) {
                                continue;
                            }
                        }
                    }
                    switch(linfo.attrType) {
                        case INT: {
                            int lvalue;
                            memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                            int rvalue;
                            if (conditions[i].bRhsIsAttr) {
                                string rrname(relation);
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
                                string rrname(relation);
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
                            string lvalue(pData+linfo.offset);
                            //memcpy(&lvalue, pData+linfo.offset, linfo.attrLength);
                            string rvalue;
                            if (conditions[i].bRhsIsAttr) {
                                string rrname(relation);
                                string raname(conditions[i].rhsAttr.attrName);
                                DataAttrInfo rinfo = attr2info[rrname+"."+raname];
                                //memcpy(&rvalue, pData+rinfo.offset, rinfo.attrLength);
                                string tmp(pData+rinfo.offset);
                                rvalue = tmp;
                            } else {
                                //memcpy(&rvalue, conditions[i].rhsValue.data, linfo.attrLength);
                                string tmp((char*)conditions[i].rhsValue.data);
                                rvalue = tmp;
                            }
                            matched = matchValue(conditions[i].op, lvalue, rvalue);
                            break;
                        }
                        default: {
                            ;
                        }
                    }
                }
            } else {
                if (conditions[i].isNULL) {
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
            rm_records.push_back(rec);
        }
    }
    CHECK_NOZERO(ix_scan.CloseScan());
    CHECK_NOZERO(ix_mgr->CloseIndex(ix_ihdl));
    CHECK_NOZERO(rm_mgr->CloseFile(rm_fhdl));
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
                      int nConditions, Condition conditions[])
{

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
    for (int i = 0; i < nConditions; i++) {
        if (!conditions[i].bRhsIsAttr) {
            string laname(conditions[i].lhsAttr.attrName);
            string lrname;
            if (conditions[i].lhsAttr.relName == NULL) {
                lrname = *(attr2rels[laname].begin());
            } else {
                string tmp(conditions[i].lhsAttr.relName);
                lrname = tmp;
            }
            if (!conditions[i].isNotNULL && !conditions[i].isNULL && !matched(attr2info[lrname+"."+laname].attrType ,conditions[i].rhsValue.type, (char*)conditions[i].rhsValue.data)) {
                return QL_DONT_MATCH;
            }
        }
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
        DataAttrInfo dataAttrInfo;
        string laname(conditions[i].lhsAttr.attrName);
        if (conditions[i].lhsAttr.relName == NULL) {
            dataAttrInfo = attr2info[*(attr2rels[laname].begin())+"."+laname];
        } else {
            dataAttrInfo = attr2info[string(conditions[i].lhsAttr.relName)+"."+laname];
        }
        AttrType lhsType = dataAttrInfo.attrType;
        AttrType rhsType;
        // 4.2 check rhsAttr && 4.3 check compatibility
        if (conditions[i].bRhsIsAttr) {
            if ((rc = CheckAttr(conditions[i].rhsAttr, attr2rels, relsCnt))) {
                return rc;
            }
            string raname(conditions[i].rhsAttr.attrName);
            if (conditions[i].rhsAttr.relName == NULL) {
                dataAttrInfo = attr2info[*(attr2rels[raname].begin())+"."+raname];
            } else {
                dataAttrInfo = attr2info[string(conditions[i].rhsAttr.relName)+"."+raname];
            }
            rhsType = dataAttrInfo.attrType;
        } else {
            rhsType = conditions[i].rhsValue.type;
        }
        if (lhsType != rhsType) {
            return QL_INCOMPATIBLE_TYPE;
        }
    }

    vector<char*> res[2];
    // scan every file
    for (int i = 0; i < nRelations; i++) {
        bool optimize = false;
        int indexNo, condNo;
        Value value;
        for (int j = 0; j < nConditions; j++) {
            string laname = string(conditions[j].lhsAttr.attrName);
            if (conditions[j].lhsAttr.relName == NULL) {
                if (attr2info.find(string(relations[i])+"."+laname) == attr2info.end()) {
                    continue;
                }
            } else {
                if (strcmp(conditions[j].lhsAttr.relName, relations[i]) == 0) {
                    if (!conditions[j].bRhsIsAttr) {
                        DataAttrInfo linfo = attr2info[string(relations[i])+"."+laname];
                        if(linfo.indexNo != -1) {
                            optimize = true;
                            indexNo = linfo.indexNo;
                            condNo = j;
                            value = conditions[j].rhsValue;
                            break;
                        }
                    }
                }
            }

        }
        vector<RM_Record> records;
        if (!optimize) {
            RM_GetRecords(relations[i], nConditions, conditions, records);
        } else {
            IX_GetRecords(relations[i], nConditions, conditions, records, indexNo, condNo);
        }
        char* pData;
        for (int j = 0; j < records.size(); j++) {
            records[j].GetData(pData);
            char* tmp = new char[records[j].GetRecordSize()];
            memcpy(tmp, pData, records[j].GetRecordSize());
            res[i].push_back(tmp);
        }
        
    }
    
    if (nRelations == 1) {
        Printer p(*sm_mgr, changedSelAttrInfo, changedSelAttrInfo.size(), attributes);
        p.PrintHeader(cout);
        for (int i = 0; i < res[0].size(); i++) {
            if (i < 10) {
                p.Print(cout, res[0][i]);
            } else {
                p.FakePrint();
            }
        }
        p.PrintFooter(cout);
    }
    // cross-products
    if (nRelations == 2) {
        Printer p(*sm_mgr, changedSelAttrInfo, changedSelAttrInfo.size(), attributes);
        p.PrintHeader(cout);
        int printedCount = 0;
        char* pData = new char[dataRelInfos[0].recordSize + dataRelInfos[1].recordSize];
        for (unsigned i = 0; i < res[0].size(); i++) {
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
                                    memcpy(&rvalue, res[1][j]+rinfo.offset, rinfo.attrLength);
                                    matched = matchValue(conditions[k].op, lvalue, rvalue);
                                    break;
                                }
                                case FLOAT: {
                                    float lvalue;
                                    memcpy(&lvalue, res[0][i]+linfo.offset, linfo.attrLength);
                                    float rvalue;
                                    memcpy(&rvalue, res[1][j]+rinfo.offset, rinfo.attrLength);
                                    matched = matchValue(conditions[k].op, lvalue, rvalue);
                                    break;
                                }
                                case STRING: {
                                    string lvalue(res[0][i]+linfo.offset);
                                    //memcpy(&lvalue, res[0][i]+linfo.offset, linfo.attrLength);
                                    string rvalue(res[1][j]+rinfo.offset);
                                    //memcpy(&rvalue, res[1][j]+rinfo.offset, rinfo.attrLength);
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
                if (matched) {
                    memcpy(pData, res[0][i], dataRelInfos[0].recordSize);
                    memcpy(pData + dataRelInfos[0].recordSize, res[1][j], dataRelInfos[1].recordSize);
                    if (printedCount < 10) {
                        p.Print(cout, pData);
                    } else {
                        p.FakePrint();
                    }
                    printedCount++;
                }
            }
        }
        delete []pData;
        p.PrintFooter(cout);
    }
    for (int i = 0; i < 2; ++i)
        for (auto x : res[i])
            delete []x;

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
                      int nValues, Value values[])
{
    RC rc;

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
    CHECK_NOZERO(sm_mgr->FindRel(relName, dataRelInfo, rid, found));
    if (!found) {
        return QL_RELATION_NOT_FOUND;
    }

    // get attr length from rel
    vector<DataAttrInfo> dataAttrInfos;
    CHECK_NOZERO(sm_mgr->FindAllAttrs(relName, dataAttrInfos));
    if (dataAttrInfos.size() != nValues) {
        return QL_INCONSISTENT_VALUE_AMOUNT;
    }
    for (int i = 0; i < dataAttrInfos.size(); i++) {
        if (!matched(dataAttrInfos[i].attrType, values[i].type, (char*)values[i].data)) {
            return QL_DONT_MATCH;
        }
    }
    DataAttrInfo last = dataAttrInfos[dataAttrInfos.size() - 1];

    IX_IndexHandle ix_idhl;
    IX_IndexScan ix_scan;
    Condition conditions[1];
    for (int i = 0; i < dataAttrInfos.size(); i++) {
        if (dataAttrInfos[i].isPrimaryKey) {
            conditions[0].op = EQ_OP;
            if (values[i].type != NULLTYPE) {
                conditions[0].rhsValue = values[i];
                vector<RM_Record> rm_records;
                CHECK_NOZERO(ix_mgr->OpenIndex(relName, dataAttrInfos[i].indexNo, ix_idhl));
                IX_GetRecords(relName, 0, conditions, rm_records, dataAttrInfos[i].indexNo, 0);
                CHECK_NOZERO(ix_mgr->CloseIndex(ix_idhl));
                if (rm_records.size() > 0) {
                    return QL_INSERT_KEY_DUPLICATED;
                }
            } else {
                return QL_ATTR_CANT_BE_NULL;
            }
            break;
        }
    }

    // insert into rm file
    RM_FileHandle rm_fhdl;
    CHECK_NOZERO(rm_mgr->OpenFile(relName, rm_fhdl));
    char* recData = new char[dataRelInfo.recordSize];
    memset(recData, 0, dataRelInfo.recordSize);
    int numBytes = (dataRelInfo.attrCount + 7) / 8;
    char* c = recData + last.offset + last.attrLength;
    RM_BitMap bitmap(numBytes, c);
    for (int i = 0; i < nValues; i++) {
        if (values[i].type == NULLTYPE) {
            if (!dataAttrInfos[i].couldBeNULL) {
                delete[] recData;
                return QL_ATTR_CANT_BE_NULL;
            }
            CHECK_NOZERO(bitmap.set(i, true));
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
            CHECK_NOZERO(bitmap.set(i, false));
        }
    }
    CHECK_NOZERO(rm_fhdl.InsertRec(recData, rid));
    CHECK_NOZERO(rm_mgr->CloseFile(rm_fhdl));

    // insert into index
    IX_IndexHandle ix_ihdl;
    for (int i = 0; i < nValues; i++) {
        if (dataAttrInfos[i].indexNo != -1) {
            bool isNull;
            CHECK_NOZERO(bitmap.isFree(i, isNull));
            if (!isNull) {
                CHECK_NOZERO(ix_mgr->OpenIndex(relName, i, ix_ihdl));
                CHECK_NOZERO(ix_ihdl.InsertEntry(values[i].data, rid));
                CHECK_NOZERO(ix_mgr->CloseIndex(ix_ihdl));
            }
        }
    }

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

    return OK_RC;
}

//
// Delete from the relName all tuples that satisfy conditions
//
RC QL_Manager::Delete(const char *relName,
                      int nConditions, Condition conditions[])
{
    if (printPara) {
        int i;

        cout << "Delete\n";

        cout << "   relName = " << relName << "\n";
        cout << "   nCondtions = " << nConditions << "\n";
        for (i = 0; i < nConditions; i++)
            cout << "   conditions[" << i << "]:" << conditions[i] << "\n";
    }

    RC rc;
    RID rid;
   
    // find dataAttrInfos, build map form name to dataAttrInfo
    vector<DataAttrInfo> dataAttrInfos;
    map<string, DataAttrInfo> attr2info;
    CHECK_NOZERO(sm_mgr->FindAllAttrs(relName, dataAttrInfos));
    if (rc)
        return rc;

    for (auto attr : dataAttrInfos) {
        attr2info[string(attr.attrName)] = attr;
    }
    DataAttrInfo last = dataAttrInfos.back();
    int numBytes = (dataAttrInfos.size() + 7) / 8;
    for (int i = 0; i < nConditions; i++) {
        if (!conditions[i].bRhsIsAttr) {
            string laname(conditions[i].lhsAttr.attrName);
            if (!conditions[i].isNULL && !conditions[i].isNotNULL && !matched(attr2info[laname].attrType ,conditions[i].rhsValue.type, (char*)conditions[i].rhsValue.data)) {
                return QL_DONT_MATCH;
            }
        }
    }

    // check each condition
    for (int i = 0; i < nConditions; i++) {
        string laname = string(conditions[i].lhsAttr.attrName);
        if (attr2info.find(laname) == attr2info.end()) {
            continue;
        }
        DataAttrInfo dataAttrInfo = attr2info[laname];
        AttrType lhsType = dataAttrInfo.attrType;
        if (conditions[i].isNULL || conditions[i].isNotNULL) {
            continue;
        }
        AttrType rhsType;
        if (conditions[i].bRhsIsAttr) {
            string raname = string(conditions[i].rhsAttr.attrName);
            if (attr2info.find(raname) == attr2info.end()) {
                continue;
            }
            dataAttrInfo = attr2info[raname];
            rhsType = dataAttrInfo.attrType;
        } else {
            rhsType = conditions[i].rhsValue.type;
        }
        if (lhsType != rhsType) {
            return QL_INCOMPATIBLE_TYPE;
        }
    }

    bool optimize = false;
    int indexNo, condNo;
    Value value;
    for (int j = 0; j < nConditions; j++) {
        if (!conditions[j].bRhsIsAttr) {
            string laname(conditions[j].lhsAttr.attrName);
            DataAttrInfo linfo = attr2info[laname];
            if(linfo.indexNo != -1) {
                optimize = true;
                indexNo = linfo.indexNo;
                condNo = j;
                value = conditions[j].rhsValue;
                break;
            }
        }
    }

    RM_FileHandle rm_fhdl;
    rm_mgr->OpenFile(relName, rm_fhdl);
    vector<RM_Record> records;
    if (!optimize) {
        RM_GetRecords(relName, nConditions, conditions, records);
    } else {
        IX_GetRecords(relName, nConditions, conditions, records, indexNo, condNo);
    }
    char* pData;
    for (int j = 0; j < records.size(); j++) {
        records[j].GetRid(rid);
        rm_fhdl.DeleteRec(rid);
    }
    rm_mgr->CloseFile(rm_fhdl);

    // delete from ix
    IX_IndexHandle ix_ihdl;
    for (int i = 0; i < dataAttrInfos.size(); i++) {
        if (dataAttrInfos[i].indexNo != -1) {
            ix_mgr->OpenIndex(relName, dataAttrInfos[i].indexNo, ix_ihdl);
            for (int j = 0; j < records.size(); j++) {
                records[j].GetData(pData);
                records[j].GetRid(rid);
                char* c = pData + last.offset + last.attrLength;
                RM_BitMap bitmap(numBytes, c);
                bool isNull;
                bitmap.isFree(i, isNull);
                if (!isNull) {
                    ix_ihdl.DeleteEntry(pData + dataAttrInfos[i].offset, rid);
                }
            }
            ix_mgr->CloseIndex(ix_ihdl);
        }
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
                      int nConditions, Condition conditions[])
{

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
    
    // find dataRelInfo
    RC rc;
    DataRelInfo dataRelInfo;
    RID rid;
    bool found;
    sm_mgr->FindRel(relName, dataRelInfo, rid, found);

    // find dataAttrInfos, build map fomr name to dataAttrInfo
    vector<DataAttrInfo> dataAttrInfos;
    map<string, DataAttrInfo> attr2info;
    sm_mgr->FindAllAttrs(relName, dataAttrInfos);
    for (auto attr : dataAttrInfos) {
        attr2info[string(attr.attrName)] = attr;
    }

    for (int i = 0; i < nConditions; i++) {
        if (!conditions[i].bRhsIsAttr) {
            string laname(conditions[i].lhsAttr.attrName);
            if (!matched(attr2info[laname].attrType ,conditions[i].rhsValue.type, (char*)conditions[i].rhsValue.data)) {
                return QL_DONT_MATCH;
            }
        }
    }
    // check each condition
    for (int i = 0; i < nConditions; i++) {
        string laname = string(conditions[i].lhsAttr.attrName);
        if (attr2info.find(laname) == attr2info.end()) {
            continue;
        }
        DataAttrInfo dataAttrInfo = attr2info[laname];
        AttrType lhsType = dataAttrInfo.attrType;
        if (conditions[i].isNULL || conditions[i].isNotNULL) {
            continue;
        }
        AttrType rhsType;
        if (conditions[i].bRhsIsAttr) {
            string raname = string(conditions[i].rhsAttr.attrName);
            if (attr2info.find(raname) == attr2info.end()) {
                continue;
            }
            dataAttrInfo = attr2info[raname];
            rhsType = dataAttrInfo.attrType;
        } else {
            rhsType = conditions[i].rhsValue.type;
        }
        if (lhsType != rhsType) {
            return QL_INCOMPATIBLE_TYPE;
        }
    }

    bool optimize = false;
    int indexNo, condNo;
    Value value;
    for (int j = 0; j < nConditions; j++) {
        if (!conditions[j].bRhsIsAttr) {
            string laname(conditions[j].lhsAttr.attrName);
            DataAttrInfo linfo = attr2info[laname];
            if(linfo.indexNo != -1) {
                optimize = true;
                indexNo = linfo.indexNo;
                condNo = j;
                value = conditions[j].rhsValue;
                break;
            }
        }
    }

    vector<RM_Record> records;
    if (!optimize) {
        RM_GetRecords(relName, nConditions, conditions, records);
    } else {
        IX_GetRecords(relName, nConditions, conditions, records, indexNo, condNo);
    }
    bool hasPrimaryKey = false;
    int primaryNo;

    for (int i = 0; i < nColumns; i++) {
        string laname(columnNames[i]);
        DataAttrInfo linfo = attr2info[laname];
        if (linfo.isPrimaryKey) {
            hasPrimaryKey = true;
            primaryNo = i;
        }
    }

    DataAttrInfo last = dataAttrInfos.back();
    int numBytes = (dataRelInfo.attrCount + 7) / 8;
    RM_FileHandle rm_fhdl;

    for (int i = 0; i < records.size(); i++) {
        rm_mgr->OpenFile(relName, rm_fhdl);
        char* recData = NULL;
        records[i].GetData(recData);

        if (hasPrimaryKey) {
            // check if the updated table conflicts
            int tmp_n = 1;
            vector<RM_Record> tmp_records;
            Condition tmp_conditions[1];
            string laname(columnNames[primaryNo]);
            DataAttrInfo linfo = attr2info[laname];
            tmp_conditions[0].lhsAttr.relName = linfo.relName;
            tmp_conditions[0].lhsAttr.attrName = linfo.attrName;
            tmp_conditions[0].rhsValue = values[primaryNo];
            tmp_conditions[0].op = EQ_OP;
            tmp_conditions[0].bRhsIsAttr = false;
            tmp_conditions[0].isNULL = false;
            tmp_conditions[0].isNotNULL = false;
            if (!optimize) {
                RM_GetRecords(relName, tmp_n, tmp_conditions, tmp_records);
            } else {
                IX_GetRecords(relName, tmp_n, tmp_conditions, tmp_records, indexNo, condNo);
            }
            if (tmp_records.size() == 1) {
                RID r1, r2;
                records[i].GetRid(r1);
                tmp_records[0].GetRid(r2);
                if (r1.pageNum != r2.pageNum || r1.slotNum != r2.slotNum) {
                    return QL_UPDATE_CONFLICT;
                }
            }
        }

        // init bitmap
        char* c = recData + last.offset + last.attrLength;
        RM_BitMap bitmap(numBytes, c);

        char* bakData = new char[records[i].GetRecordSize()];
        memcpy(bakData, recData, records[i].GetRecordSize());
        RM_BitMap bakBitmap(numBytes, c);
        for (int i = 0; i < nColumns; i++) {
            string laname(columnNames[i]);
            DataAttrInfo linfo = attr2info[laname];
            if (values[i].type == NULLTYPE) {
                if (!linfo.couldBeNULL) {
                    rm_mgr->CloseFile(rm_fhdl);
                    delete[] bakData;
                    return QL_ATTR_CANT_BE_NULL;
                }
                bitmap.set(i, true);
            } else {
                memcpy(recData+linfo.offset, values[i].data, linfo.attrLength);
                bitmap.set(i, false);
            }
        }
        rm_fhdl.UpdateRec(records[i]);
        rm_mgr->CloseFile(rm_fhdl);
        // update to ix
        IX_IndexHandle ix_ihdl;
        char* pData = NULL;
        for (int j = 0; j < dataAttrInfos.size(); j++) {
            if (dataAttrInfos[j].indexNo != -1) {
                ix_mgr->OpenIndex(relName, dataAttrInfos[j].indexNo, ix_ihdl);
                records[i].GetData(pData);
                records[i].GetRid(rid);
                bool isNull;
                bakBitmap.isFree(j, isNull);
                if (!isNull) {
                    ix_ihdl.DeleteEntry(bakData + dataAttrInfos[j].offset, rid);
                }
                bitmap.isFree(j, isNull);
                if (!isNull) {
                    ix_ihdl.InsertEntry(pData + dataAttrInfos[j].offset, rid);
                }
                ix_mgr->CloseIndex(ix_ihdl);
            }
        }
        delete[] bakData;

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
