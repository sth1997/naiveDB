#include "sm.h"
#include <unistd.h>
#include <set>

using namespace std;
#define CHECK_NONZERO(x) { \
    if ((rc = x)) { \
        if (rc < 0) \
            return rc; \
        else \
            printf("ix_ihdl WARNING: %d \n", rc); } \
}

#define PRINT_NONZERO(x) { \
    if ((rc = x)) { \
        if (rc < 0) \
            printf("ERROR: %d \n", rc); \
        else \
            printf("ix_ihdl print WARNING: %d \n", rc); } \
}

SM_Manager::SM_Manager(RM_Manager& rm_Manager, IX_Manager& ix_Manager)
    :rmm(rm_Manager), ixm(ix_Manager), DBOpen(false)
{
    //memset(cwd, 0, sizeof(cwd));
}

SM_Manager::~SM_Manager()
{
    //TODO: handle open db
}

RC SM_Manager::ValidName(const char* name)
{
    if (name == NULL)
        return SM_NULLDBNAME;
    if (strlen(name) > MAXNAME)
        return SM_NAMETOOLONG;
    return OK_RC;
}

RC SM_Manager::CreateDb(const char* dbName)
{
    RC rc;
    CHECK_NONZERO(ValidName(dbName));
    if (DBOpen)
        return SM_DBALREADYOPEN;

    if (access(dbName, F_OK) == 0)
        return SM_DBEXISTS;

    char command[128];
    sprintf(command, "mkdir %s", dbName);
    if (system(command))
        return SM_MKDIRERROR;

    //get into the db's directory
    if (chdir(dbName))
        return SM_CHDIRERROR;

    //create and open catalog
    CHECK_NONZERO(rmm.CreateFile("relcat", sizeof(DataRelInfo)));
    CHECK_NONZERO(rmm.OpenFile("relcat", relcat));
    CHECK_NONZERO(rmm.CreateFile("attrcat", sizeof(DataAttrInfo)));
    CHECK_NONZERO(rmm.OpenFile("attrcat", attrcat));

    // insert "relcat" and "attrcat" into the relcat file as two relations
    DataRelInfo relcatRel;
    DataRelInfo attrcatRel;
    strcpy(relcatRel.relName, "relcat");
    strcpy(attrcatRel.relName, "attrcat");
    relcatRel.recordSize = sizeof(DataRelInfo);
    attrcatRel.recordSize = sizeof(DataAttrInfo);
    relcatRel.attrCount = DataRelInfo::memberNumber();
    attrcatRel.attrCount = DataAttrInfo::memberNumber();

    RID rid;
    CHECK_NONZERO(relcat.InsertRec((char*) &relcatRel, rid));
    CHECK_NONZERO(relcat.InsertRec((char*) &attrcatRel, rid));

    // insert the attributes of DataRelInfo into the attrcat file
    DataAttrInfo attr;
    strcpy(attr.relName, "relcat");
    strcpy(attr.attrName, "recordSize");
    attr.attrLength = sizeof(int);
    attr.attrType = INT;
    attr.indexNo = -1;
    attr.offset = offsetof(DataRelInfo, recordSize);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    strcpy(attr.relName, "relcat");
    strcpy(attr.attrName, "attrCount");
    attr.attrLength = sizeof(int);
    attr.attrType = INT;
    attr.indexNo = -1;
    attr.offset = offsetof(DataRelInfo, attrCount);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    strcpy(attr.relName, "relcat");
    strcpy(attr.attrName, "relName");
    attr.attrLength = MAXNAME + 1;
    attr.attrType = STRING;
    attr.indexNo = -1;
    attr.offset = offsetof(DataRelInfo, relName);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    // insert the attributes of DataAttrInfo into the attrcat file
    strcpy(attr.relName, "attrcat");
    strcpy(attr.attrName, "offset");
    attr.attrLength = sizeof(int);
    attr.attrType = INT;
    attr.indexNo = -1;
    attr.offset = offsetof(DataAttrInfo, offset);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    strcpy(attr.relName, "attrcat");
    strcpy(attr.attrName, "attrType");
    attr.attrLength = sizeof(AttrType);
    attr.attrType = INT;
    attr.indexNo = -1;
    attr.offset = offsetof(DataAttrInfo, attrType);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    strcpy(attr.relName, "attrcat");
    strcpy(attr.attrName, "attrLength");
    attr.attrLength = sizeof(int);
    attr.attrType = INT;
    attr.indexNo = -1;
    attr.offset = offsetof(DataAttrInfo, attrLength);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    strcpy(attr.relName, "attrcat");
    strcpy(attr.attrName, "indexNo");
    attr.attrLength = sizeof(int);
    attr.attrType = INT;
    attr.indexNo = -1;
    attr.offset = offsetof(DataAttrInfo, indexNo);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    strcpy(attr.relName, "attrcat");
    strcpy(attr.attrName, "relName");
    attr.attrLength = MAXNAME + 1;
    attr.attrType = STRING;
    attr.indexNo = -1;
    attr.offset = offsetof(DataAttrInfo, relName);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    strcpy(attr.relName, "relcat");
    strcpy(attr.attrName, "attrName");
    attr.attrLength = MAXNAME + 1;
    attr.attrType = STRING;
    attr.indexNo = -1;
    attr.offset = offsetof(DataAttrInfo, attrName);
    CHECK_NONZERO(attrcat.InsertRec((char*) &attr, rid));

    CHECK_NONZERO(rmm.CloseFile(relcat));
    CHECK_NONZERO(rmm.CloseFile(attrcat));

    if(chdir("../"))
        return SM_CHDIRERROR;

    return OK_RC;
}

RC SM_Manager::DestroyDb(const char* dbName)
{
    RC rc;
    CHECK_NONZERO(ValidName(dbName));
    if (DBOpen)
        return SM_DBALREADYOPEN;

    if (chdir(dbName))
        return SM_NOSUCHDB;

    if(chdir("../"))
        return SM_CHDIRERROR;
    
    char command[128];
    sprintf(command, "rm -r %s", dbName);
    if (system(command))
        return SM_REMOVEERROR;

    return OK_RC;
}

RC SM_Manager::OpenDb(const char* dbName)
{
    RC rc;
    CHECK_NONZERO(ValidName(dbName));
    if (DBOpen)
        return SM_DBALREADYOPEN;
    
    /*if (getcwd(cwd, sizeof(cwd)) != cwd)
        return SM_GETCWDERROR;*/
    
    if (chdir(dbName))
        return SM_NOSUCHDB;

    //open catalog
    CHECK_NONZERO(rmm.OpenFile("relcat", relcat));
    CHECK_NONZERO(rmm.OpenFile("attrcat", attrcat));
    DBOpen = true;
    return OK_RC;
}

RC SM_Manager::CloseDb()
{
    RC rc;
    if (!DBOpen)
        return SM_DBNOTOPEN;
    CHECK_NONZERO(rmm.CloseFile(relcat));
    CHECK_NONZERO(rmm.CloseFile(attrcat));

    if (chdir("../"))
        return SM_CHDIRERROR;
    DBOpen = false;
    return OK_RC;
}

// Find the relation named relName in relcat.
// If not found, set found = false
RC SM_Manager::FindRel(const char* relName, DataRelInfo& rel, RID& rid, bool& found)
{
    found = false;
    RC rc;
    CHECK_NONZERO(ValidName(relName));
    if (!DBOpen)
        return SM_DBNOTOPEN;
    
    RM_FileScan scan;
    CHECK_NONZERO(scan.OpenScan(relcat, STRING, MAXNAME + 1, offsetof(DataRelInfo, relName), EQ_OP, (void*) relName));

    RM_Record record;
    rc = scan.GetNextRec(record);
    if (rc == RM_EOF) // not found
        return OK_RC;
    if(rc)  // error
        return rc;
    
    CHECK_NONZERO(scan.CloseScan());
    found = true;
    DataRelInfo* relpointer;
    CHECK_NONZERO(record.GetData((char*&) relpointer));
    rel = *relpointer;
    CHECK_NONZERO(record.GetRid(rid));
    return OK_RC;
}

// Find the attribute named attrName which belongs to the relation named relName
// If not found, set found = false
RC SM_Manager::FindAttr(const char* relName, const char* attrName, DataAttrInfo& attrinfo, RID& rid, bool& found)
{
    found = false;
    RC rc;
    CHECK_NONZERO(ValidName(relName));
    CHECK_NONZERO(ValidName(attrName));
    if (!DBOpen)
        return SM_DBNOTOPEN;

    RM_FileScan scan;
    CHECK_NONZERO(scan.OpenScan(attrcat, STRING, MAXNAME + 1, offsetof(DataAttrInfo, attrName), EQ_OP, (void*) attrName));

    RM_Record record;
    rc = scan.GetNextRec(record);
    while (rc != RM_EOF)
    {
        if (rc) // error
            return rc;
        DataAttrInfo* attrpointer;
        CHECK_NONZERO(record.GetData((char*&) attrpointer));
        if (strcmp(attrpointer->relName, relName) == 0)
        {
            found = true;
            attrinfo = *attrpointer;
            CHECK_NONZERO(record.GetRid(rid));
            break;
        }
    }
    CHECK_NONZERO(scan.CloseScan());
    return OK_RC; // whatever found or not
}

RC SM_Manager::CreateTable(const char* relName, int attrCount, AttrInfo* attributes)
{
    RC rc;
    CHECK_NONZERO(ValidName(relName));
    if (!DBOpen)
        return SM_DBNOTOPEN;
    if (attrCount <= 0 || attributes == NULL)
        return SM_BADTABLEPARA;
    if (strcmp(relName, "attrcat") == 0 || strcmp(relName, "relcat") == 0)
        return SM_BADTABLEPARA;

    // check wether the table already exists
    bool foundRel = false;
    DataRelInfo tmp_relinfo;
    RID tmp_rid;
    CHECK_NONZERO(FindRel(relName, tmp_relinfo, tmp_rid, foundRel));
    if (foundRel)
        return SM_BADTABLEPARA;
    
    RID rid;
    set<string> attrNames; // used to check whether there are two attributes with the same name
    attrNames.clear();
    DataAttrInfo* attrinfo = new DataAttrInfo[attrCount];
    int totalLength = 0;
    for (int i = 0; i < attrCount; ++i)
    {
        if (attributes[i].attrType == INT || attributes[i].attrType == FLOAT)
        {
            if (attributes[i].attrLength != 4)
                return SM_BADTABLEPARA;
        }
        else if (attributes[i].attrType == STRING)
        {
            if (attributes[i].attrLength < 1 || attributes[i].attrLength > MAXSTRINGLEN)
                return SM_BADTABLEPARA;
        }
        else
            return SM_BADTABLEPARA;
        attrinfo[i] = DataAttrInfo(attributes[i], totalLength, -1, relName);
        totalLength += attrinfo[i].attrLength;
        if (attrNames.find(string(attrinfo[i].attrName)) == attrNames.end())
            attrNames.insert(string(attrinfo[i].attrName));
        else // there are two attributes with the same name
        {
            delete []attrinfo;
            return SM_BADTABLEPARA;
        }
    }

    for (int i = 0; i < attrCount; ++i)
        CHECK_NONZERO((attrcat.InsertRec((char*) &attrinfo[i], rid)));
    delete []attrinfo;

    CHECK_NONZERO(rmm.CreateFile(relName, totalLength));
    DataRelInfo relinfo;
    relinfo.attrCount = attrCount;
    relinfo.recordSize = totalLength;
    strcpy(relinfo.relName, relName);
    CHECK_NONZERO(relcat.InsertRec((char*) &relinfo, rid));
    return OK_RC;
}

RC SM_Manager::DropTable(const char* relName)
{
    RC rc;
    CHECK_NONZERO(ValidName(relName));
    if (!DBOpen)
        return SM_DBNOTOPEN;
    if (strcmp(relName, "attrcat") == 0 || strcmp(relName, "relcat") == 0)
        return SM_BADTABLEPARA;
    
    
    DataRelInfo relinfo;
    bool found = false;
    RID rid;
    CHECK_NONZERO(FindRel(relName, relinfo, rid, found));
    if (!found)
        return SM_NOSUCHTABLE;
    CHECK_NONZERO(rmm.DestroyFile(relName));
    CHECK_NONZERO(relcat.DeleteRec(rid));
    
    RM_FileScan scan;
    RM_Record record;
    DataAttrInfo* attrinfo;
    CHECK_NONZERO(scan.OpenScan(attrcat, STRING, MAXNAME + 1, offsetof(DataAttrInfo, relName), EQ_OP, (void*) relName));
    rc = scan.GetNextRec(record);
    while (rc != RM_EOF)
    {
        if (rc)
            return rc;
        CHECK_NONZERO(record.GetData((char*&) attrinfo));
        assert(strcmp(attrinfo->relName, relName) == 0);
        if (attrinfo->indexNo != -1)
            CHECK_NONZERO(ixm.DestroyIndex(relName, attrinfo->offset));
        record.GetRid(rid);
        CHECK_NONZERO(attrcat.DeleteRec(rid));
        rc = scan.GetNextRec(record);
    }
    CHECK_NONZERO(scan.CloseScan());
    return OK_RC;
}

RC SM_Manager::CreateIndex(const char* relName, const char* attrName)
{
    RC rc;
    CHECK_NONZERO(ValidName(relName));
    CHECK_NONZERO(ValidName(attrName));
    if (!DBOpen)
        return SM_DBNOTOPEN;
    
    DataAttrInfo attrinfo;
    RID rid;
    bool found = false;
    CHECK_NONZERO(FindAttr(relName, attrName, attrinfo, rid, found));
    if (!found)
        return SM_NOSUCHATTR;
    if (attrinfo.indexNo != -1)
        return SM_INDEXEXISTS;

    CHECK_NONZERO(ixm.CreateIndex(relName, attrinfo.offset, attrinfo.attrType, attrinfo.attrLength));
    attrinfo.indexNo = attrinfo.offset;
    //update attrcat
    RM_Record record;
    record.SetData((char*) &attrinfo, sizeof(DataAttrInfo), rid);
    CHECK_NONZERO(attrcat.UpdateRec(record));

    // insert all records into the new index
    IX_IndexHandle indexHandle;
    CHECK_NONZERO(ixm.OpenIndex(relName, attrinfo.indexNo, indexHandle));
    RM_FileHandle fileHandle;
    CHECK_NONZERO(rmm.OpenFile(relName, fileHandle));
    RM_FileScan scan;
    CHECK_NONZERO(scan.OpenScan(fileHandle, attrinfo.attrType, attrinfo.attrLength, attrinfo.offset, NO_OP, NULL));

    rc = scan.GetNextRec(record);
    while (rc != RM_EOF)
    {
        if (rc) // error
            return rc;
        char* data;
        CHECK_NONZERO(record.GetData(data));
        CHECK_NONZERO(record.GetRid(rid));
        indexHandle.InsertEntry(data + attrinfo.offset, rid);
        CHECK_NONZERO(scan.GetNextRec(record));
    }

    CHECK_NONZERO(scan.CloseScan());
    CHECK_NONZERO(rmm.CloseFile(fileHandle));
    CHECK_NONZERO(ixm.CloseIndex(indexHandle));
    return OK_RC;
}

RC SM_Manager::DropIndex(const char* relName, const char* attrName)
{
    RC rc;
    CHECK_NONZERO(ValidName(relName));
    CHECK_NONZERO(ValidName(attrName));
    if (!DBOpen)
        return SM_DBNOTOPEN;

    DataAttrInfo attrinfo;
    RID rid;
    bool found = false;
    CHECK_NONZERO(FindAttr(relName, attrName, attrinfo, rid, found));
    if (!found)
        return SM_NOSUCHATTR;
    if (attrinfo.indexNo == -1)
        return SM_INDEXEXISTS;

    CHECK_NONZERO(ixm.DestroyIndex(relName, attrinfo.indexNo));
    //update attrcat
    attrinfo.indexNo = -1;
    RM_Record record;
    record.SetData((char*) &attrinfo, sizeof(DataAttrInfo), rid);
    CHECK_NONZERO(attrcat.UpdateRec(record));
    return OK_RC;
}