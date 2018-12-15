#include "sm.h"
#include <unistd.h>
#include <stddef.h>
#include <dirent.h>
#include <set>
#include <vector>

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

RC SM_Manager::ValidName(const char* name) const
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

    strcpy(attr.relName, "attrcat");
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
        rc = scan.GetNextRec(record);
    }
    CHECK_NONZERO(scan.CloseScan());
    return OK_RC; // whatever found or not
}

// Find all attributes belong to relation named relName
RC SM_Manager::FindAllAttrs(const char* relName, vector<DataAttrInfo>& attrs)
{
    bool foundRel = false;
    RC rc;
    CHECK_NONZERO(ValidName(relName));
    if (!DBOpen)
        return SM_DBNOTOPEN;
    // check wether the table already exists
    DataRelInfo tmp_relinfo;
    RID tmp_rid;
    CHECK_NONZERO(FindRel(relName, tmp_relinfo, tmp_rid, foundRel));
    if (!foundRel)
        return SM_NOSUCHTABLE;
    
    attrs.clear();
    RM_FileScan scan;
    CHECK_NONZERO(scan.OpenScan(attrcat, STRING, MAXNAME + 1, offsetof(DataAttrInfo, relName), EQ_OP, (void*) relName));
    RM_Record record;
    rc = scan.GetNextRec(record);
    while (rc != RM_EOF)
    {
        if (rc)
            return rc;
        DataAttrInfo* attrinfo;
        CHECK_NONZERO(record.GetData((char*&) attrinfo));
        attrs.push_back(*attrinfo);
        rc = scan.GetNextRec(record);
    }
    scan.CloseScan();
    if (attrs.size() != tmp_relinfo.attrCount)
        return SM_ATTRNUMINCORRECT;
    return OK_RC;
}

//check whether the relation named relName exists
RC SM_Manager::CheckRel(const char* relName, bool& found)
{
    found = false;
    RC rc;
    CHECK_NONZERO(ValidName(relName));
    if (!DBOpen)
        return SM_DBNOTOPEN;
    DataRelInfo relInfo;
    RID rid;
    CHECK_NONZERO(FindRel(relName, relInfo, rid, found));
    return OK_RC;
}

//check whether the relation named relName and the attribute named attrName exist
RC SM_Manager::CheckAttr(const char* relName, const char* attrName, bool& found)
{
    found = false;
    RC rc;
    CHECK_NONZERO(ValidName(relName));
    CHECK_NONZERO(ValidName(attrName));
    if (!DBOpen)
        return SM_DBNOTOPEN;
    DataAttrInfo attrInfo;
    RID rid;
    CHECK_NONZERO(FindAttr(relName, attrName, attrInfo, rid, found));
    return OK_RC;
}

//check whether the relation or attribute of the left hand side or right hand side exists
// If they all exist, but the types of them mismatched, the "found" is also set false.
RC SM_Manager::CheckCond(const Condition& cond, bool& found)
{
    found = false;
    if (cond.op < NO_OP || cond.op > GE_OP)
        return SM_INVALIDCOMPOP;
    RC rc;
    CHECK_NONZERO(ValidName(cond.lhsAttr.relName));
    CHECK_NONZERO(ValidName(cond.lhsAttr.attrName));
    DataAttrInfo attrInfo1;
    RID rid;
    CHECK_NONZERO(FindAttr(cond.lhsAttr.relName, cond.lhsAttr.attrName, attrInfo1, rid, found));
    if (!found) // not found
        return OK_RC;
    found = false;

    if (cond.bRhsIsAttr)
    {
        DataAttrInfo attrInfo2;
        CHECK_NONZERO(ValidName(cond.rhsAttr.relName));
        CHECK_NONZERO(ValidName(cond.rhsAttr.attrName));
        CHECK_NONZERO(FindAttr(cond.rhsAttr.relName, cond.rhsAttr.attrName, attrInfo2, rid, found));
        if (!found) // not found
            return OK_RC;
        if (attrInfo1.attrType != attrInfo2.attrType)
        {
            found = false;
            return SM_TYPEMISMATCH;
        }
        return OK_RC;
    }
    else
    {
        if (attrInfo1.attrType != cond.rhsValue.type)
        {
            found = false;
            return SM_TYPEMISMATCH;
        }
        return OK_RC;
    }
}

RC SM_Manager::IsAttrIndexed(const char* relName, const char* attrName, bool& indexed)
{
    RC rc;
    indexed = false;
    if (!DBOpen)
        return SM_DBNOTOPEN;
    CHECK_NONZERO(ValidName(relName));
    CHECK_NONZERO(ValidName(attrName));
    DataAttrInfo attrInfo;
    RID rid;
    bool found = false;
    CHECK_NONZERO(FindAttr(relName, attrName, attrInfo, rid, found));
    if (found && attrInfo.indexNo != -1)
        indexed = true;
    else
        indexed = false;
    return OK_RC;
}

// find the relation which has the attr named attr.attrName
// the input attr.relName must be null, the user needs to call free() for attr.relName
RC SM_Manager::FindRelForAttr(RelAttr& attr, int numRel, const char* const possibleRels[])
{
    RC rc;
    if (!DBOpen)
        return SM_DBNOTOPEN;
    if (attr.relName != NULL)
        return OK_RC;

    bool found = false;;
    bool alreadyFound = false;
    for (int i = 0; i < numRel; ++i)
    {
        CHECK_NONZERO(CheckAttr(possibleRels[i], attr.attrName, found));
        if (found && !alreadyFound)
        {
            alreadyFound = true;
            attr.relName = strdup(possibleRels[i]);
        }
        else if (found && alreadyFound)
        {
            free(attr.relName);
            attr.relName = NULL;
            return SM_MULTIPLERELSASATISFY;
        }
    }

    if (!alreadyFound)
        return SM_NOSUCHENTRY;
    else
        return OK_RC;
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
            {
                delete []attrinfo;
                return SM_BADTABLEPARA;
            }
        }
        else if (attributes[i].attrType == STRING)
        {
            if (attributes[i].attrLength < 1 || attributes[i].attrLength > MAXSTRINGLEN)
            {
                delete []attrinfo;
                return SM_BADTABLEPARA;
            }
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
        rc = scan.GetNextRec(record);
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
        return SM_NOSUCHINDEX;

    CHECK_NONZERO(ixm.DestroyIndex(relName, attrinfo.indexNo));
    //update attrcat
    attrinfo.indexNo = -1;
    RM_Record record;
    record.SetData((char*) &attrinfo, sizeof(DataAttrInfo), rid);
    CHECK_NONZERO(attrcat.UpdateRec(record));
    return OK_RC;
}

// Print all dbs and elations in each of them. For relations, only print the relName.
RC SM_Manager::PrintDBs()
{
    if (DBOpen)
        return SM_DBALREADYOPEN;
    DIR *dir;
    struct dirent *ptr;
    //char base[1000];
    vector<string> dbNames;

    if ((dir=opendir("./")) == NULL)
        return SM_OPENDIRERROR;
    while ((ptr=readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;
        /*else if(ptr->d_type == 8)    ///file
            printf("d_name:%s/%s\n",basePath,ptr->d_name);
        else if(ptr->d_type == 10)    ///link file
            printf("link_file:%s/%s\n",basePath,ptr->d_name);
        else */
        if(ptr->d_type == 4)    ///dir
        {
            dbNames.push_back(string(ptr->d_name));
            /*memset(base,'\0',sizeof(base));
            strcpy(base,basePath);
            strcat(base,"/");
            strcat(base,ptr->d_name);
            readFileList(base);*/
        }
    }
    closedir(dir);

    RM_FileScan scan;
    RC rc;
    for (auto dbName : dbNames)
    {
        printf("-----------------------------------------------------------\n");
        printf("DB NAME: %s\n", dbName.c_str());
        printf("TABLES:\n");
        CHECK_NONZERO(OpenDb(dbName.c_str()));
        CHECK_NONZERO(scan.OpenScan(relcat, STRING, MAXNAME + 1, offsetof(DataRelInfo, relName), NO_OP, NULL));
        RM_Record record;
        rc = scan.GetNextRec(record);
        DataRelInfo* relinfo;
        while (rc != RM_EOF)
        {
            if (rc) // error
                return rc;
            CHECK_NONZERO(record.GetData((char*&) relinfo));
            printf("          %s\n", relinfo->relName);
            rc = scan.GetNextRec(record);
        }
        scan.CloseScan();
        CHECK_NONZERO(CloseDb());
    }
    return OK_RC;
}

// Print all tables in this db and attributes in each table.
RC SM_Manager::PrintTables()
{
    if (!DBOpen)
        return SM_DBNOTOPEN;
    RM_FileScan scan;
    RC rc;
    CHECK_NONZERO(scan.OpenScan(relcat, STRING, MAXNAME + 1, offsetof(DataRelInfo, relName), NO_OP, NULL));
    RM_Record record;
    rc = scan.GetNextRec(record);
    DataRelInfo* relinfo;
    while (rc != RM_EOF)
    {
        if (rc) // error
            return rc;
        CHECK_NONZERO(record.GetData((char*&) relinfo));
        printf("-------------------------------------------------------------\n");
        CHECK_NONZERO(PrintTable(relinfo->relName));
        rc = scan.GetNextRec(record);
    }
    CHECK_NONZERO(scan.CloseScan());
    return OK_RC;
}

void SM_Manager::PrintAttrType(AttrType attrType)
{
    if (attrType == INT)
        printf("INT");
    else if (attrType == FLOAT)
        printf("FLOAT");
    else if (attrType == STRING)
        printf("STRING");
    else
        assert(1);
}

// Print all attributes in table named relName
RC SM_Manager::PrintTable(const char* relName)
{
    RC rc;
    CHECK_NONZERO(ValidName(relName));
    if (!DBOpen)
        return SM_DBNOTOPEN;
    
    bool foundRel = false;
    DataRelInfo tmp_relinfo;
    RID tmp_rid;
    CHECK_NONZERO(FindRel(relName, tmp_relinfo, tmp_rid, foundRel));
    if (!foundRel)
        return SM_NOSUCHTABLE;
    
    printf("TABLE NAME: %s\n", relName);

    vector<DataAttrInfo> attrs;
    CHECK_NONZERO(FindAllAttrs(relName, attrs));
    for (auto attr : attrs)
    {
        printf("       %s         ", attr.attrName);
        PrintAttrType(attr.attrType);
        printf("(%d)", attr.attrLength);
        if (attr.indexNo == -1)
            printf("         NOT INDEXED\n");
        else
            printf("         INDEXED\n");
    }
    return OK_RC;
}