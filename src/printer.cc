//
// printer.cc
//

// This file contains the interface for the Printer class and some
// functions that will be used by both the SM and QL components.

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include "printer.h"
#include "rm.h"
#include "sm.h"

using namespace std;

#define CHECK_NOZERO(x)            \
    {                              \
        if ((rc = x)) {            \
            printf("printer warning; %d\n", rc);    \
            if (rc < 0) {          \
                return; \
            }                      \
        }                          \
    }

//
// void Spaces(int maxLength, int printedSoFar)
//
// This method will output some spaces so that print entry will align everythin
// nice and neat.
//
void Spaces(int maxLength, int printedSoFar)
{
    for (int i = printedSoFar; i < maxLength; i++)
        cout << " ";
}

//
// ------------------------------------------------------------------------------
//

//
// Printer
//
// This class handles the printing of tuples.
//
//  DataAttrInfo - describes all of the attributes. Defined
//      within sm.h
//  attrCount - the number of attributes
//
Printer::Printer(SM_Manager &smm, vector<DataAttrInfo> attributes_, const int attrCount_, vector<vector<DataAttrInfo> > allAttributes_)
{
    sm_mgr = &smm;
    attrCount = attrCount_;
    attributes = new DataAttrInfo[attrCount];

    for (int i=0; i < attrCount; i++)
        attributes[i] = attributes_[i];
    
    allAttributes = allAttributes_;

    // Number of tuples printed
    iCount = 0;

    // Figure out what the header information will look like.  Normally,
    // we can just use the attribute name, but if that appears more than
    // once, then we should use "relation.attribute".

    // this line broke when using CC
    // changing to use malloc and free instead of new and delete
    // psHeader = (char **) new (char *)[attrCount];
    psHeader = (char**)malloc(attrCount * sizeof(char*));

    // Also figure out the number of spaces between each attribute
    spaces = new int[attrCount];

    for (int i=0; i < attrCount; i++ ) {
        // Try to find the attribute in another column
        int bFound = 0;
        psHeader[i] = new char[MAXPRINTSTRING];
        memset(psHeader[i],0,MAXPRINTSTRING);

        for (int j=0; j < attrCount; j++)
            if (j != i &&
                strcmp(attributes[i].attrName,
                       attributes[j].attrName) == 0) {
                bFound = 1;
                break;
            }

        if (bFound)
            sprintf(psHeader[i], "%s.%s",
                    attributes[i].relName, attributes[i].attrName);
        else
            strcpy(psHeader[i], attributes[i].attrName);

        if (attributes[i].attrType==STRING)
            spaces[i] = min(attributes[i].attrLength, MAXPRINTSTRING);
        else
            spaces[i] = max(12, strlen(psHeader[i]));

        // We must subtract out those characters that will be for the
        // header.
        spaces[i] -= strlen(psHeader[i]);

        // If there are negative (or zero) spaces, then insert a single
        // space.
        if (spaces[i] < 1) {
            // The psHeader will give us the space we need
            spaces[i] = 0;
            strcat(psHeader[i]," ");
        }
    }
}


//
// Destructor
//
Printer::~Printer()
{
    for (int i = 0; i < attrCount; i++)
        delete [] psHeader[i];

    delete [] spaces;
    //delete [] psHeader;
    free (psHeader);
    delete [] attributes;
}

//
// PrintHeader
//
void Printer::PrintHeader( ostream &c ) const
{
    int dashes = 0;
    int iLen;
    int i,j;

    for (i = 0; i < attrCount; i++) {
        // Print out the header information name
        c << psHeader[i];
        iLen = strlen(psHeader[i]);
        dashes += iLen;

        for (j = 0; j < spaces[i]; j++)
            c << " ";

        dashes += spaces[i];
    }

    c << "\n";
    for (i = 0; i < dashes; i++) c << "-";
    c << "\n";
}

//
// PrintFooter
//
void Printer::PrintFooter(ostream &c) const
{
    c << "\n";
    c << iCount << " tuple(s).\n";
}

//
// Print
//
//  data - this is an array of void *.  This print routine is used by
//  the QL Layer.
//
//  Unfortunately, this is essentially the same as the other Print
//  routine.
//
void Printer::Print(ostream &c, const void * const data[])
{
    char str[MAXPRINTSTRING], strSpace[50];
    int i, a;
    float b;

    // Increment the number of tuples printed
    iCount++;

    for (i = 0; i<attrCount; i++) {
        if (attributes[i].attrType == STRING) {
            // We will only print out the first MAXNAME+10 characters of
            // the string value.
            memset(str,0,MAXPRINTSTRING);

            if (attributes[i].attrLength>MAXPRINTSTRING) {
                strncpy(str, (char *)data[i], MAXPRINTSTRING-1);
                str[MAXPRINTSTRING-3] ='.';
                str[MAXPRINTSTRING-2] ='.';
                c << str;
                Spaces(MAXPRINTSTRING, strlen(str));
            } else {
                strncpy(str, (char *)data[i], attributes[i].attrLength);
                c << str;
                if (attributes[i].attrLength < (int) strlen(psHeader[i]))
                    Spaces(strlen(psHeader[i]), strlen(str));
                else
                    Spaces(attributes[i].attrLength, strlen(str));
            }
        }
        if (attributes[i].attrType == INT) {
            memcpy (&a, data[i], sizeof(int));
            sprintf(strSpace, "%d",a);
            c << a;
            if (strlen(psHeader[i]) < 12)
                Spaces(12, strlen(strSpace));
            else
                Spaces(strlen(psHeader[i]), strlen(strSpace));
        }
        if (attributes[i].attrType == FLOAT) {
            memcpy (&b, data[i], sizeof(float));
            sprintf(strSpace, "%f",b);
            c << strSpace;
            if (strlen(psHeader[i]) < 12)
                Spaces(12, strlen(strSpace));
            else
                Spaces(strlen(psHeader[i]), strlen(strSpace));
        }
    }
    c << "\n";
}

//
// Print
//
//  data - the actual data for the tuple to be printed
//
//  The routine tries to make things line up nice, however no
//  attempt is made to keep the tuple constrained to some number of
//  characters.
//
void Printer::Print(ostream &c, const char * const data)
{
    RC rc;
    char str[MAXPRINTSTRING], strSpace[50];
    int a;
    float b;
    int totalOffset = 0;
    // Increment the number of tuples printed
    iCount++;

    for (auto oneAttributes : allAttributes) {
        // init bitmap
        int numBytes = (oneAttributes.size() + 7) / 8;
        DataAttrInfo last = oneAttributes.back();
        int recLength = 0;
        for (auto attr : oneAttributes) {
            recLength += attr.attrLength;
        }
        recLength += numBytes;

        char* tmp = new char[recLength];
        memcpy(tmp, data + totalOffset, recLength);
        RM_BitMap bitmap(numBytes, tmp + last.offset + last.attrLength);

        if (data == NULL)
            return;

        for (auto attr : oneAttributes) {
            for (unsigned i = 0; i<attrCount; i++) {
                if (strcmp(attr.relName, attributes[i].relName) == 0 && strcmp(attr.attrName, attributes[i].attrName) == 0) {
                    int pos;
                    DataAttrInfo tmp;
                    bool tmpf;
                    RID rid;
                    CHECK_NOZERO(sm_mgr->FindAttr(attributes[i].relName, attributes[i].attrName, tmp, rid, tmpf, &pos));
                    bool isNull = false;
                    bitmap.isFree(pos, isNull);
                    if (isNull) {
                        c << "NULL";
                        if (strlen(psHeader[i]) < 12)
                            Spaces(12, strlen("NULL"));
                        else
                            Spaces(strlen(psHeader[i]), strlen(strSpace));
                    } else {
                        if (attributes[i].attrType == STRING) {
                            // We will only print out the first MAXNAME+10 characters of
                            // the string value.
                            memset(str,0,MAXPRINTSTRING);

                            if (attributes[i].attrLength>MAXPRINTSTRING) {
                                strncpy(str, data+totalOffset+attributes[i].offset, MAXPRINTSTRING-1);
                                str[MAXPRINTSTRING-3] ='.';
                                str[MAXPRINTSTRING-2] ='.';
                                c << str;
                                Spaces(MAXPRINTSTRING, strlen(str));
                            } else {
                                strncpy(str, data+totalOffset+attributes[i].offset, attributes[i].attrLength);
                                c << str;
                                if (attributes[i].attrLength < (int) strlen(psHeader[i]))
                                    Spaces(strlen(psHeader[i]), strlen(str));
                                else
                                    Spaces(attributes[i].attrLength, strlen(str));
                            }
                        }
                        if (attributes[i].attrType == INT) {
                            memcpy (&a, (data+totalOffset+attributes[i].offset), sizeof(int));
                            sprintf(strSpace, "%d",a);
                            c << a;
                            if (strlen(psHeader[i]) < 12)
                                Spaces(12, strlen(strSpace));
                            else
                                Spaces(strlen(psHeader[i]), strlen(strSpace));
                        }
                        if (attributes[i].attrType == FLOAT) {
                            memcpy (&b, (data+totalOffset+attributes[i].offset), sizeof(float));
                            sprintf(strSpace, "%f",b);
                            c << strSpace;
                            if (strlen(psHeader[i]) < 12)
                                Spaces(12, strlen(strSpace));
                            else
                                Spaces(strlen(psHeader[i]), strlen(strSpace));
                        }
                    }
                }
            }
        }

        totalOffset += recLength;

        delete[] tmp;
    }

    c << "\n";
}

void Printer::FakePrint() {
    iCount++;
}
