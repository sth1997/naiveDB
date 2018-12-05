#include "ix_internal.h"
#include "ix.h"

#include <iostream>
#include <cstring>
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

IX_IndexHandle::IX_IndexHandle():
    fileOpen(false), headerChanged(false), rootNode(NULL), path(NULL), childPos(NULL), fileHandle(NULL), largestKey(NULL)
{
    fileHeader.height = 0;
}

IX_IndexHandle::~IX_IndexHandle()
{
    RC rc;
    if (rootNode)
    {
        PRINT_NONZERO(fileHandle->MarkDirty(fileHeader.rootPage));
        PRINT_NONZERO(fileHandle->UnpinPage(fileHeader.rootPage));
        delete rootNode;
        rootNode = NULL;
    }
    if (childPos)
    {
        delete []childPos;
        childPos = NULL;
    }
    if (path)
    {
        // path[0] = rootNode, so we don't delete it once again
        for (int i = 1; i < fileHeader.height; ++i)
        {
            if (path[i])
                PRINT_NONZERO(DeleteNode(path[i]));
        }
        delete []path;
        path = NULL;
    }
    if (fileHandle)
    {
        delete fileHandle;
        fileHandle = NULL;
    }
    if (largestKey)
    {
        delete [] (char*)largestKey;
        largestKey = NULL;
    }

}

RC IX_IndexHandle::DeleteNode(BTreeNode* &node)
{
    if (node)
    {
        RC rc;
        if (node->nodeChanged())
            CHECK_NONZERO(fileHandle->MarkDirty(node->getPageNum()));
        CHECK_NONZERO(fileHandle->UnpinPage(node->getPageNum()));
        delete node;
        node = NULL;
    }
    return OK_RC;
}

RC IX_IndexHandle::SetFileHeader() const
{
    PF_PageHandle pageHandle;
    RC rc;
    CHECK_NONZERO(fileHandle->GetThisPage(0, pageHandle));
    char* buf;
    CHECK_NONZERO(pageHandle.GetData(buf));
    memcpy(buf, &fileHeader, sizeof(fileHeader));
    CHECK_NONZERO(fileHandle->MarkDirty(0));
    CHECK_NONZERO(fileHandle->UnpinPage(0));
    return OK_RC;
}

RC IX_IndexHandle::GetFileHeader()
{
    PF_PageHandle pageHandle;
    RC rc;
    CHECK_NONZERO(fileHandle->GetThisPage(0, pageHandle));
    char* buf;
    CHECK_NONZERO(pageHandle.GetData(buf));
    memcpy(&fileHeader, buf, sizeof(fileHeader));
    CHECK_NONZERO(fileHandle->UnpinPage(0));
    return OK_RC;
}

RC IX_IndexHandle::AllocatePage(PageNum& pageNum)
{
    RC rc;
    CHECK_NONZERO(IsValid());
    
    PF_PageHandle pageHandle;
    CHECK_NONZERO(fileHandle->AllocatePage(pageHandle));
    CHECK_NONZERO(pageHandle.GetPageNum(pageNum));
    CHECK_NONZERO(fileHandle->MarkDirty(pageNum));
    CHECK_NONZERO(fileHandle->UnpinPage(pageNum));
    headerChanged = true;
    return OK_RC;
}

RC IX_IndexHandle::IsValid() const
{
    if (fileHandle == NULL)
        return IX_INVALID;
    if (fileHeader.height > 0)
        if (fileHeader.rootPage <= 0 || rootNode == NULL || path == NULL)
            return IX_INVALID;
    return OK_RC;
}

RC IX_IndexHandle::SetHeight(int h)
{
    RC rc;
    if (path)
    {
        for (int i = 1; i < fileHeader.height; ++i)
            CHECK_NONZERO(DeleteNode(path[i]));
        delete []path;
        path = NULL;
    }
    if (childPos)
    {
        delete []childPos;
        childPos = NULL;
    }

    fileHeader.height = h;
    headerChanged = true;
    path = new BTreeNode*[h];
    path[0] = rootNode;
    for (int i = 1; i < h; ++i)
        path[i] = NULL;
    
    childPos = new int[h - 1];
    for (int i = 0; i < h - 1; ++i)
        childPos[i] = -1;
    return OK_RC;
}

RC IX_IndexHandle::FetchNode(PageNum page, BTreeNode* &node) const
{
    RC rc;
    CHECK_NONZERO(IsValid());
    PF_PageHandle pageHandle;
    CHECK_NONZERO(fileHandle->GetThisPage(page, pageHandle));
    node = new BTreeNode(fileHeader.attrType, fileHeader.attrLength, pageHandle, false);
    return OK_RC;
}

//return a pointer to the leaf node which the key should be put in
RC IX_IndexHandle::FindLeaf(const void* pData, const RID& rid, BTreeNode* &node)
{
    RC rc;
    CHECK_NONZERO(IsValid());
    assert(rootNode != NULL);
    if (fileHeader.height == 1)
    {
        path[0] = rootNode;
        node = rootNode;
        return OK_RC;
    }

    for (int i = 1; i < fileHeader.height; ++i)
    {
        int pos = path[i - 1]->findKeyGE((char*)pData, rid);
        if (pos == path[i - 1]->getNum()) //if pData is larger than any other key, just use the largest key pos
            --pos;
        CHECK_NONZERO(DeleteNode(path[i]));
        CHECK_NONZERO(FetchNode(path[i - 1]->getPage(pos), path[i]));
        childPos[i - 1] = pos;
    }
    node = path[fileHeader.height - 1];
    return OK_RC;
}

RC IX_IndexHandle::FindLargestLeaf(BTreeNode* &node)
{
    assert(rootNode != NULL);
    RC rc;
    CHECK_NONZERO(IsValid());
    path[0] = rootNode;
    if (fileHeader.height == 1)
    {
        node = rootNode;
        return OK_RC;
    }

    for (int i = 1; i < fileHeader.height; ++i)
    {
        PageNum page = path[i-1]->getPage(path[i - 1]->getNum() - 1);
        CHECK_NONZERO(DeleteNode(path[i]));
        CHECK_NONZERO(FetchNode(page, path[i]));
        childPos[i - 1] = path[i - 1]->getNum() - 1;
    }
    node = path[fileHeader.height - 1];
    return OK_RC;
}

RC IX_IndexHandle::FindSmallestLeaf(BTreeNode* &node)
{
    assert(rootNode != NULL);
    RC rc;
    CHECK_NONZERO(IsValid());
    path[0] = rootNode;
    if (fileHeader.height == 1)
    {
        node = rootNode;
        return OK_RC;
    }

    for (int i = 1; i < fileHeader.height; ++i)
    {
        PageNum page = path[i-1]->getPage(0);
        CHECK_NONZERO(DeleteNode(path[i]));
        CHECK_NONZERO(FetchNode(page, path[i]));
        childPos[i - 1] = 0;
    }
    node = path[fileHeader.height - 1];
    return OK_RC;
}

RC IX_IndexHandle::Open(PF_FileHandle* handle)
{
    if (fileOpen || fileHandle != NULL)
        return IX_OPENFILETWICE;
    if (handle == NULL)
        return IX_INVALIDFILEHANDLE;
    fileOpen = true;
    fileHandle = new PF_FileHandle();
    *fileHandle = *handle;
    headerChanged = false;

    RC rc;
    //get file header
    CHECK_NONZERO(GetFileHeader());

    bool createRoot = true;
    if (fileHeader.height > 0)
    {
        SetHeight(fileHeader.height); // init path and childPos
        createRoot = false;
    }
    else
    {
        PageNum pageNum;
        CHECK_NONZERO(AllocatePage(pageNum));
        fileHeader.rootPage = pageNum;
        SetHeight(1);
        headerChanged = true;
    }
    PF_PageHandle rootPageHandle;
    CHECK_NONZERO(fileHandle->GetThisPage(fileHeader.rootPage, rootPageHandle));
    rootNode = new BTreeNode(fileHeader.attrType, fileHeader.attrLength, rootPageHandle, createRoot);
    path[0] = rootNode;
    CHECK_NONZERO(IsValid());
    largestKey = new char[fileHeader.attrLength];
    if (!createRoot)
    {
        BTreeNode* node;
        CHECK_NONZERO(FindLargestLeaf(node));
        if (node->getNum() > 0)
        {
            node->copyKey(node->getNum() - 1, largestKey);
            largestRID = node->getRID(node->getNum() - 1);
        }
    }
    return OK_RC;
}

RC IX_IndexHandle::UpdateLargest()
{
    RC rc;
    CHECK_NONZERO(IsValid());
    memcpy(largestKey, rootNode->getKey(rootNode->getNum() - 1), fileHeader.attrLength);
    largestRID = rootNode->getRID(rootNode->getNum() - 1);
    return OK_RC;
}

RC IX_IndexHandle::InsertEntry(void *pData, const RID& rid)
{
    RC rc;
    CHECK_NONZERO(IsValid());
    if (pData == NULL)
        return IX_NULLKEYDATA;

    bool newLargest = false;
    BTreeNode* node;
    CHECK_NONZERO(FindLeaf(pData, rid, node));
    int tmp_pos = node->findKey((char*)pData, rid);
    if (tmp_pos != -1)
        return IX_ENTRYEXISTS;
    
    if (node->getNum() == 0 || node->CMP((char*)pData, largestKey, rid, largestRID) > 0)
    {
        newLargest = true;
    }

    bool full = false;
    rc = node->insert((char*)pData, rid, rid.pageNum);
    if (rc == IX_NODEISFULL)
        full = true;
    else
        if (rc != OK_RC)
            return rc;
    
    if (newLargest)
    {
        for (int i = 0; i < fileHeader.height - 1; ++i)
        {
            int pos = path[i]->findKey(largestKey, largestRID);
            if (pos != -1)
            {
                assert(pos == path[i]->getNum() - 1);
                path[i]->setKey(pos, (char*) pData);
                path[i]->setRID(pos, rid);
            }
            else
                assert(0);
        }
        memcpy(largestKey, pData, fileHeader.attrLength);
        largestRID = rid;
    }

    char* lastKey = new char[fileHeader.attrLength];
    memcpy(lastKey, pData, fileHeader.attrLength);
    RID lastRID = rid;
    PageNum lastPage = rid.pageNum;
    BTreeNode* newNode = NULL;
    int height = fileHeader.height - 1; //leaf's height
    while (full)
    {
        PageNum p;
        CHECK_NONZERO(AllocatePage(p));
        PF_PageHandle pageHandle;
        CHECK_NONZERO(fileHandle->GetThisPage(p, pageHandle));
        newNode = new BTreeNode(fileHeader.attrType, fileHeader.attrLength, pageHandle, true);
        CHECK_NONZERO(node->split(*newNode));
        PageNum nextPage = newNode->getNext();
        if (nextPage != -1)
        {
            BTreeNode* nextNode;
            CHECK_NONZERO(FetchNode(nextPage, nextNode));
            nextNode->setPre(newNode->getPageNum());
            CHECK_NONZERO(DeleteNode(nextNode));
        }

        //select one node to insert
        if (node->CMP(lastKey, node->getLargestKey(), lastRID, node->getLargestRID()) > 0)
            newNode->insert(lastKey, lastRID, lastPage);
        else
            node->insert(lastKey, lastRID, lastPage);

        --height;
        if (height < 0)
            break;
        //father node stores this node at pos
        int pos = childPos[height];
        BTreeNode* father = path[height];
        CHECK_NONZERO(father->remove(NULL, RID(-1, -1), pos));
        CHECK_NONZERO(father->insert(node->getLargestKey(), node->getLargestRID(), node->getPageNum()));
        rc = father->insert(newNode->getLargestKey(), newNode->getLargestRID(), newNode->getPageNum());
        if (rc == IX_NODEISFULL)
            full = true;
        else
        {
            full = false;
            if (rc != OK_RC)
                return rc;
        }
        node = father;
        //because the newNode will be deleted, its page will be unpin, so we need to copy this key, instead of getting its pointer
        newNode->copyKey(newNode->getNum() - 1, lastKey);
        lastRID = newNode->getLargestRID();
        lastPage = newNode->getPageNum();

        CHECK_NONZERO(DeleteNode(newNode));
    }
    delete []lastKey;

    if (height >= 0)
        return OK_RC;

    // the root node was also splited, so we need to create a new root node
    assert(node == rootNode);
    headerChanged = true;
    PageNum page;
    CHECK_NONZERO(AllocatePage(page));
    PF_PageHandle pageHandle;
    CHECK_NONZERO(fileHandle->GetThisPage(page, pageHandle));
    rootNode = new BTreeNode(fileHeader.attrType, fileHeader.attrLength, pageHandle, true);
    path[0] = rootNode;
    CHECK_NONZERO(rootNode->insert(node->getLargestKey(), node->getLargestRID(), node->getPageNum()));
    CHECK_NONZERO(rootNode->insert(newNode->getLargestKey(), newNode->getLargestRID(), newNode->getPageNum()));

    fileHeader.rootPage = page;
    CHECK_NONZERO(DeleteNode(newNode));
    CHECK_NONZERO(DeleteNode(node)); // delete the old root
    SetHeight(fileHeader.height + 1);
    return OK_RC;
}

RC IX_IndexHandle::DisposePage(PageNum pageNum)
{
    RC rc;
    CHECK_NONZERO(IsValid());
    CHECK_NONZERO(fileHandle->DisposePage(pageNum));
    return OK_RC;
}

RC IX_IndexHandle::DeleteEntry(void* pData, const RID& rid)
{
    if (pData == NULL)
        return IX_NULLKEYDATA;
    RC rc;
    CHECK_NONZERO(IsValid());

    BTreeNode* node;
    CHECK_NONZERO(FindLeaf(pData, rid, node));
    int pos = node->findKey((char*)pData, rid);
    if (pos == -1)
        return IX_ENTRYNOTEXIST;
    
    //update the largest key in path
    if(pos == node->getNum() - 1) // key is the largest key in this node, it also appears in its father node and even ancestor nodes
    {
        for (int i = fileHeader.height - 2; i >= 0; --i)
        {
            int pos = path[i]->findKey((char*) pData, rid);
            if (pos != -1)
            {
                char* lastKey;
                RID lastRID;
                lastKey = path[i + 1]->getLargestKey();
                lastRID = path[i + 1]->getLargestRID();
                if (node->CMP((char*)pData, lastKey, rid, lastRID) == 0)
                {
                    int pos = path[i + 1]->getNum() - 2;
                    if (pos < 0) //underflow
                        continue;
                    lastKey = path[i + 1]->getKey(pos);
                    lastRID = path[i + 1]->getRID(pos);
                }
                path[i]->setKey(pos, lastKey);
                path[i]->setRID(pos, lastRID);
                if (pos != path[i]->getNum() - 1)
                    break;
            }
            else
                assert(0);
        }
    }
    
    CHECK_NONZERO(node->remove((char*)pData, rid));
    int height = fileHeader.height - 1;
    while (node->getNum() == 0)
    {
        --height;
        if (height < 0) break;
        BTreeNode* father = path[height];
        CHECK_NONZERO(father->remove(NULL, RID(-1, -1), childPos[height]));
        PageNum nextPage = node->getNext();
        PageNum prePage = node->getPre();
        if (nextPage != -1)
        {
            BTreeNode* nextNode;
            CHECK_NONZERO(FetchNode(nextPage, nextNode));
            nextNode->setPre(prePage);
            CHECK_NONZERO(DeleteNode(nextNode));
        }
        if (prePage != -1)
        {
            BTreeNode* preNode;
            CHECK_NONZERO(FetchNode(prePage, preNode));
            preNode->setNext(nextPage);
            CHECK_NONZERO(DeleteNode(preNode));
        }
        PageNum nodePage = node->getPageNum();
        assert(node == path[height + 1]);
        CHECK_NONZERO(DeleteNode(node));
        path[height + 1] = NULL;
        CHECK_NONZERO(DisposePage(nodePage));
        node = father;
    }
    if (height >= 0)
    {
        UpdateLargest();
        return OK_RC;
    }
    assert(node == rootNode);
    assert(node->getNum() == 0);
    // any open index needs an root, so we can't delete root and SetHeight(0)
    CHECK_NONZERO(SetHeight(1));
    return OK_RC;
}

RC IX_IndexHandle::ForcePages()
{
    RC rc;
    CHECK_NONZERO(IsValid());
    if (rootNode)
    {
        PRINT_NONZERO(fileHandle->MarkDirty(fileHeader.rootPage));
        rootNode->setUnchanged();
    }
    if (path)
        for (int i = 1; i < fileHeader.height; ++i)
            if (path[i] && path[i]->nodeChanged())
            {
                CHECK_NONZERO(fileHandle->MarkDirty(path[i]->getPageNum()));
                path[i]->setUnchanged();
            }
    CHECK_NONZERO(fileHandle->ForcePages(-1));
    return OK_RC;
}

RC IX_IndexHandle::Print() const
{
    RC rc;
    CHECK_NONZERO(IsValid());
    printf("Root page = %d\n", fileHeader.rootPage);
    printf("height = %d\n", fileHeader.height);
    printf("max key num = %d\n", rootNode->getMaxKeyNum());
    return OK_RC;
}