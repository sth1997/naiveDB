#include "ix_internal.h"
#include "ix.h"
#include <string>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <iostream>
using namespace std;

// Constructor
IX_Manager::IX_Manager(PF_Manager &pfm) : pfm(pfm) {
}

// Destructor
IX_Manager::~IX_Manager() {
    // Nothing to free
}


RC IX_Manager::CreateIndex(const char *fileName, int indexNo,
                           AttrType attrType, int attrLength) {
    return OK_RC;
}

RC IX_Manager::DestroyIndex(const char *fileName, int indexNo) {
    return OK_RC;
}

RC IX_Manager::OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &indexHandle) {
    return OK_RC;
}

RC IX_Manager::CloseIndex(IX_IndexHandle &indexHandle) {
    return OK_RC;
}
