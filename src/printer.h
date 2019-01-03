//
// printer.h
//

// This file contains the interface for the Printer class and some
// functions that will be used by both the SM and QL components.

#ifndef _HELPER
#define _HELPER

#include <iostream>
#include <cstring>
#include "naivedb.h"      // For definition of MAXNAME
#include "catalog.h"

#define MAXPRINTSTRING  ((2*MAXNAME) + 5)

// Print some number of spaces
void Spaces(int maxLength, int printedSoFar);

class Printer {
public:
    // Constructor.  Takes as arguments an array of attributes along with
    // the length of the array.
    Printer(SM_Manager &smm, std::vector<DataAttrInfo> attributes, const int attrCount, std::vector<std::vector<DataAttrInfo> > all_attributes);
    ~Printer();

    void PrintHeader(std::ostream &c) const;

    // Two flavors for the Print routine.  The first takes a char* to the
    // data and is useful when the data corresponds to a single record in
    // a table -- since in this situation you can just send in the
    // RecData.  The second will be useful in the QL layer.
    void Print(std::ostream &c, const char * const data);
    void Print(std::ostream &c, const void * const data[]);

    void PrintFooter(std::ostream &c) const;

    void FakePrint();

private:
    DataAttrInfo *attributes;
    std::vector<std::vector<DataAttrInfo> > allAttributes;
    int attrCount;

    // An array of strings for the header information
    char **psHeader;
    // Number of spaces between each attribute
    int *spaces;

    // The number of tuples printed
    int iCount;

    SM_Manager* sm_mgr;
};


#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#endif
