#include "ix_internal.h"
#include "ix.h"
#include <iostream>
#include <cstring>
using namespace std;

#define CHECK_NOZERO(x)            \
    {                              \
        if ((rc = x)) {            \
            PF_PrintError(rc);     \
            if (rc < 0) {          \
                return IX_PFERROR; \
            }                      \
        }                          \
    }

// Constructor
IX_IndexScan::IX_IndexScan() {
    // Set open scan flag to false
    scanOpen = false;
    fetched = false;
    desc = false;
}

// Destructor
IX_IndexScan::~IX_IndexScan() {
    // Nothing to free
}

int IX_IndexScan::getIntegerValue(char* recData) {
    int recordValue;
    char* attrPointer = recData;
    memcpy(&recordValue, attrPointer, sizeof(recordValue));
    return recordValue;
}

float IX_IndexScan::getFloatValue(char* recData) {
    float recordValue;
    char* attrPointer = recData;
    memcpy(&recordValue, attrPointer, sizeof(recordValue));
    return recordValue;
}

string IX_IndexScan::getStringValue(char* recData) {
    string recordValue = "";
    char* attrPointer = recData;
    for (int i = 0; i < ix_ihdl->GetAttrLength(); i++) {
        recordValue += attrPointer[i];
    }
    return recordValue;
}

template<typename T>
bool IX_IndexScan::matchKey(T keyValue, T givenValue) {
    switch(compOp) {
        case NO_OP:
            return true;
            break;
        case EQ_OP:
            return keyValue == givenValue;
            break;
        case LT_OP:
            return keyValue < givenValue;
            break;
        case GT_OP:
            return keyValue > givenValue;
            break;
        case LE_OP:
            return keyValue <= givenValue;
            break;
        case GE_OP:
            return keyValue >= givenValue;
            break;
        case NE_OP:
            return keyValue != givenValue;
            break;
        default:
            break;
    }
    return false;
}

RC IX_IndexScan::OpenScan(IX_IndexHandle &indexHandle, CompOp compOp,
                          void *value, ClientHint  pinHint) {
    if (scanOpen) {
        return IX_HANDLEOPEN;
    }
    if (compOp == NE_OP) {
        return IX_DISALLOW_NE;
    }

    // Store the class variables
    this->ix_ihdl = &indexHandle;
    this->compOp = compOp;
    this->value = value;
    this->pinHint = pinHint;

    scanOpen = true;

    // Set up current pointers based on btree
    RC rc;
    RID rid(0, 0);
    switch(compOp) {
        case NO_OP:
        case LT_OP:
        case LE_OP:
        case NE_OP:
            CHECK_NOZERO(ix_ihdl->FindSmallestLeaf(curNode));
            curPos = 0;
            break;
        case EQ_OP:
            CHECK_NOZERO(ix_ihdl->FindLeaf(value, rid, curNode));
            curPos = 0;
            break;
        case GT_OP:
        case GE_OP:
            CHECK_NOZERO(ix_ihdl->FindLargestLeaf(curNode));
            curPos = curNode->getNum() - 1;
            desc = true;
            break;
    }
    return OK_RC;
}

RC IX_IndexScan::GetNextEntry(RID &rid) {
    while (curNode != NULL) {
        for (int i = curPos; (!desc && i < curNode->getNum()) || (desc && i >= 0); (!desc && i++) || (desc && i--)) {
            // printf("pn=%d, kn=%d, i=%d\n", curNode->getPageNum(), curNode->getNum(), i);
            char* key = curNode->getKey(i);
            bool keyMatch = false;
            switch (ix_ihdl->GetAttrType()) {
                case INT: {
                    int keyValue = getIntegerValue(key);
                    int givenValue = *static_cast<int*>(value);
                    // printf("%d %d %d\n", keyValue, compOp, givenValue);
                    keyMatch = matchKey(keyValue, givenValue);
                    break;
                }
                case FLOAT: {
                    float keyValue = getFloatValue(key);
                    float givenValue = *static_cast<float*>(value);
                    keyMatch = matchKey(keyValue, givenValue);
                    break;
                }
                case STRING: {
                    string keyValue = getStringValue(key);
                    char* givenValueChar = static_cast<char*>(value);
                    string givenValue(givenValueChar);
                    keyMatch = matchKey(keyValue, givenValue);
                    break;
                }
                default:
                    ;
            }
            if (keyMatch) {
                // printf("find at %d\n", i);
                rid = curNode->getRID(i);
                curPos = desc ? i - 1 : i + 1;
                return OK_RC;
            }
        }
        PageNum pageNum = desc ? curNode->getPre() : curNode->getNext();
        if (fetched) {
            // printf("fetched\n");
            ix_ihdl->DeleteNode(curNode);
        } else {
            // printf("not fetched\n");
            curNode = NULL;
        }
        if (pageNum != -1) {
            ix_ihdl->FetchNode(pageNum, curNode);
            fetched = true;
            if (curNode != NULL) {
                curPos = desc ? curNode->getNum() - 1 : 0;
            }
        }
    }
    return IX_EOF;
}

RC IX_IndexScan::CloseScan() {
    // Return error if the scan is closed
    if (!scanOpen) {
        return IX_SCAN_CLOSED;
    }

    // Set scan open flag to FALSE
    scanOpen = FALSE;
    if (fetched)
       ix_ihdl->DeleteNode(curNode);
    else
        curNode = NULL;
    curPos = -1;
    // Return OK
    return OK_RC;
}
