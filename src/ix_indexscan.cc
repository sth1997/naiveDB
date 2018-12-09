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
    scanOpen = FALSE;
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
    for (int i = 0; i < ix_ihdl.GetAttrLength(); i++) {
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

RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp,
                          void *value, ClientHint  pinHint) {
    if (scanOpen) {
        return IX_HANDLEOPEN;
    }
    if (compOp == NE_OP) {
        return IX_DISALLOW_NE;
    }

    // Store the class variables
    this->ix_ihdl = indexHandle;
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
            CHECK_NOZERO(ix_ihdl.FindSmallestLeaf(curNode));
            break;
        case EQ_OP:
            CHECK_NOZERO(ix_ihdl.FindLeaf(value, rid, curNode));
            break;
        case GT_OP:
        case GE_OP:
            CHECK_NOZERO(ix_ihdl.FindLargestLeaf(curNode));
            break;
    }
    curPos = 0;
    return OK_RC;
}

RC IX_IndexScan::GetNextEntry(RID &rid) {
    while (curNode != NULL) {
        for (int i = curPos; i < curNode->getNum(); i++) {
            char* key = curNode->getKey(i);
            bool keyMatch = false;
            switch (ix_ihdl.GetAttrType()) {
                case INT: {
                    int keyValue = getIntegerValue(key);
                    int givenValue = *static_cast<int*>(value);
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
            curPos = i;
            if (keyMatch) {
                rid = curNode->getRID(i);
                return OK_RC;
            }
        }
        curPos = 0;
        PageNum pageNum = curNode->getNext();
        ix_ihdl.UnPin(curNode->getPageNum());
        delete curNode;
        curNode = NULL;
        ix_ihdl.FetchNode(pageNum, curNode);
        if (curNode != NULL) {
            ix_ihdl.Pin(curNode->getPageNum());
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
    curNode = NULL;
    curPos = -1;
    // Return OK
    return OK_RC;
}
