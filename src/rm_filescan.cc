#include <cassert>
#include <cerrno>
#include <cstdio>
#include <iostream>
#include "rm.h"
#include "rm_internal.h"

#define CHECK_NOZERO(x)            \
    {                              \
        if ((rc = x)) {            \
            PF_PrintError(rc);     \
            if (rc < 0) {          \
                return RM_PFERROR; \
            }                      \
        }                          \
    }

using namespace std;

RM_FileScan::RM_FileScan() {
    scanOpen = false;
}

RM_FileScan::~RM_FileScan() {}

RC RM_FileScan::OpenScan(const RM_FileHandle& fileHandle,
                         AttrType attrType,
                         int attrLength,
                         int attrOffset,
                         CompOp compOp,
                         void* value,
                         ClientHint pinHint) {
    if (attrType != INT && attrType != FLOAT && attrType != STRING) {
        return RM_INVALID_ATTRIBUTE;
    }

    if (compOp != NO_OP && compOp != EQ_OP && compOp != NE_OP &&
        compOp != LT_OP && compOp != GT_OP && compOp != LE_OP &&
        compOp != GE_OP) {
        return RM_INVALID_OPERATOR;
    }

    if ((attrType == INT || attrType == FLOAT) && attrLength != 4) {
        return RM_ATTRIBUTE_NOT_CONSISTENT;
    }
    if (attrType == STRING) {
        if (attrLength < 1 || attrLength > MAXSTRINGLEN) {
            return RM_ATTRIBUTE_NOT_CONSISTENT;
        }
    }
    if (compOp != NO_OP && value == NULL) {
        compOp = NO_OP;
    }

    // Store the class variables
    this->rm_fhdl = &fileHandle;
    this->attrType = attrType;
    this->attrLength = attrLength;
    this->attrOffset = attrOffset;
    this->compOp = compOp;
    this->value = value;
    this->pinHint = pinHint;

    scanOpen = true;

    // rm_fhdl -> pf_fhdl -> pf_phdl -> headerPageNum
    RC rc;
    PF_FileHandle* pf_fhdl = fileHandle.GetPFFileHandle();
    PF_PageHandle pf_phdl;
    PageNum headerPageNum;
    CHECK_NOZERO(pf_fhdl->GetFirstPage(pf_phdl));
    CHECK_NOZERO(pf_phdl.GetPageNum(headerPageNum));

    // headerPageNum -> pageNum
    PageNum pageNum;
    bool pageFound = true;
    if ((rc = pf_fhdl->GetNextPage(headerPageNum, pf_phdl))) {
        if (rc == PF_EOF) {
            pageNum = RM_NO_FREE_PAGE;
            pageFound = false;
        } else {
            RM_PrintError(rc);
            if (rc < 0) {
                return RM_PFERROR;
            }
        }
    }
    if (pageFound) {
        CHECK_NOZERO(pf_phdl.GetPageNum(pageNum));
    }

    // set pageNum and slotNum
    this->pageNum = pageNum;
    this->slotNum = 0;
    CHECK_NOZERO(pf_fhdl->UnpinPage(headerPageNum));
    if (pageFound) {
        CHECK_NOZERO(pf_fhdl->UnpinPage(pageNum));
    }

    return OK_RC;
}

int RM_FileScan::getIntegerValue(char* recData) {
    int recordValue;
    char* attrPointer = recData + attrOffset;
    memcpy(&recordValue, attrPointer, sizeof(recordValue));
    return recordValue;
}

float RM_FileScan::getFloatValue(char* recData) {
    float recordValue;
    char* attrPointer = recData + attrOffset;
    memcpy(&recordValue, attrPointer, sizeof(recordValue));
    return recordValue;
}

string RM_FileScan::getStringValue(char* recData) {
    string recordValue = "";
    char* attrPointer = recData + attrOffset;
    for (int i = 0; i < attrLength; i++) {
        recordValue += attrPointer[i];
    }
    return recordValue;
}

template <typename T>
bool RM_FileScan::matchRecord(T recordValue, T givenValue) {
    switch (compOp) {
        case NO_OP:
            return true;
            break;
        case EQ_OP:
            return recordValue == givenValue;
            break;
        case LT_OP:
            return recordValue < givenValue;
            break;
        case GT_OP:
            return recordValue > givenValue;
            break;
        case LE_OP:
            return recordValue <= givenValue;
            break;
        case GE_OP:
            return recordValue >= givenValue;
            break;
        case NE_OP:
            return recordValue != givenValue;
            break;
        default:
            break;
    }
    return false;
}

RC RM_FileScan::GetNextRec(RM_Record& rec) {
    if (!scanOpen) {
        return RM_SCAN_CLOSED;
    }
    if (pageNum == RM_NO_FREE_PAGE) {
        return RM_EOF;
    }
    RC rc;
    PF_FileHandle* pf_fhdl = rm_fhdl->GetPFFileHandle();
    bool recordMatch = false;
    while (!recordMatch) {
        // get bitmap
        RM_PageHeader rm_phdr;
        PF_PageHandle pf_phdl;
        CHECK_NOZERO(pf_fhdl->GetThisPage(pageNum, pf_phdl));
        rm_fhdl->GetPageHeader(pf_phdl, rm_phdr);
        RM_BitMap bitmap(rm_phdr.freeSlotsNum, rm_phdr.freeSlots);
        bool isFree;
        bitmap.isFree(slotNum, isFree);
        if (!isFree) {
            char* recData;
            rm_fhdl->GetSlotPointer(pf_phdl, slotNum, recData);
            
            if (compOp != NO_OP)
            {
                switch (attrType) {
                    case INT: {
                        int recordValue = getIntegerValue(recData);
                        int givenValue = *static_cast<int*>(value);
                        recordMatch = matchRecord(recordValue, givenValue);
                        break;
                    }
                    case FLOAT: {
                        float recordValue = getFloatValue(recData);
                        float givenValue = *static_cast<float*>(value);
                        recordMatch = matchRecord(recordValue, givenValue);
                        break;
                    }
                    case STRING: {
                        string recordValue(recData + attrOffset);
                        char* givenValueChar = static_cast<char*>(value);
                        string givenValue(givenValueChar);
                        recordMatch = matchRecord(recordValue, givenValue);
                        break;
                    }
                    default:;
                }
            }
            else
                recordMatch = true;

            if (recordMatch) {
                // printf("record matched!\n");
                RM_FileHeader rm_fhdr = rm_fhdl->GetRMFileHeader();
                int recordSize = rm_fhdr.recordSize;
                char* newData = new char[recordSize];
                memcpy(newData, recData, recordSize);
                RID newRid(pageNum, slotNum);
                rec.SetData(newData, recordSize, newRid);
                delete []newData;
            }
        }

        // printf("slot1=%d, slot2=%d\n", slotNum, rm_phdr.totalSlotsNum);
        if (slotNum < rm_phdr.totalSlotsNum) {
            slotNum++;
        } else {
            CHECK_NOZERO(pf_fhdl->UnpinPage(pageNum));
            if ((rc = pf_fhdl->GetNextPage(pageNum, pf_phdl))) {
                if (rc == PF_EOF) {
                    pageNum = RM_NO_FREE_PAGE;
                    if (recordMatch) {
                        return OK_RC;
                    } else {
                        return RM_EOF;
                    }
                } else {
                    RM_PrintError(rc);
                    if (rc < 0) {
                        return RM_PFERROR;
                    }
                }
            }
            CHECK_NOZERO(pf_phdl.GetPageNum(pageNum));
            slotNum = 0;
        }
    }
    if (pinHint == NO_HINT) {
        CHECK_NOZERO(pf_fhdl->UnpinPage(pageNum));
    }

    return OK_RC;
}

RC RM_FileScan::CloseScan() {
    if (!scanOpen) {
        return RM_SCAN_CLOSED;
    }
    scanOpen = false;

    return OK_RC;
}
