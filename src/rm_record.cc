#include "rm.h"
#include <cstring>

RM_Record::RM_Record() : data(NULL), recordSize(-1)
{}

RM_Record::~RM_Record()
{
    if (data)
    {
        delete []data;
        recordSize = -1;
    }
}

RC RM_Record::SetData(char* data, int recordSize, const RID& rid)
{
    if (this->data)
        delete [](this->data);
    this->data = new char[recordSize];
    memcpy(this->data, data, recordSize);
    this->recordSize = recordSize;
    this->rid = rid;
    return OK_RC;
}

RC RM_Record::GetData(char *&pData) const
{
    if (recordSize == -1)
        return RM_NORECORD;
    pData = this->data;
    return OK_RC;
}

RC RM_Record::GetRid(RID &rid) const
{
    if (recordSize == -1)
        return RM_NORECORD;
    rid = this->rid;
    return OK_RC;
}

RC RM_Record::IsNull(const DataRelInfo& relinfo, int bitPos, bool& isNull) const
{
    if (relinfo.recordSize != this->recordSize)
        return RM_RECORDSIZEMISMATCH;
    int offset = relinfo.recordSize - (relinfo.attrCount + 7) / 8;
    RM_BitMap bitmap((relinfo.attrCount + 7) / 8, data + offset);
    RC rc = bitmap.isFree(bitPos, isNull);
    return rc;
}

RC RM_Record::SetNull(const DataRelInfo& relinfo, int bitPos)
{
    if (relinfo.recordSize != this->recordSize)
        return RM_RECORDSIZEMISMATCH;
    int offset = relinfo.recordSize - (relinfo.attrCount + 7) / 8;
    RM_BitMap bitmap((relinfo.attrCount + 7) / 8, data + offset);
    RC rc = bitmap.set(bitPos, true);
    return rc;
}

RC RM_Record::SetNotNull(const DataRelInfo& relinfo, int bitPos)
{
    if (relinfo.recordSize != this->recordSize)
        return RM_RECORDSIZEMISMATCH;
    int offset = relinfo.recordSize - (relinfo.attrCount + 7) / 8;
    RM_BitMap bitmap((relinfo.attrCount + 7) / 8, data + offset);
    RC rc = bitmap.set(bitPos, false);
}