#include "ix_internal.h"
#include "ix.h"
#include <iostream>
#include <cstring>
using namespace std;

// Constructor
IX_IndexScan::IX_IndexScan() {
    // Set open scan flag to false
    scanOpen = FALSE;
}

// Destructor
IX_IndexScan::~IX_IndexScan() {
    // Nothing to free
}

RC IX_IndexScan::OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp,
                          void *value, ClientHint  pinHint) {
    return OK_RC;
}

RC IX_IndexScan::GetNextEntry(RID &rid) {
    return OK_RC;
}

RC IX_IndexScan::CloseScan() {
    // Return error if the scan is closed
    if (!scanOpen) {
        return IX_SCAN_CLOSED;
    }

    // Set scan open flag to FALSE
    scanOpen = FALSE;

    // Return OK
    return OK_RC;
}
