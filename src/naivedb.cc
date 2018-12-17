//
// redbase.cc
//
// Author: Jason McHugh (mchughj@cs.stanford.edu)
//
// This shell is provided for the student.

#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "naivedb.h"
#include "rm.h"
#include "sm.h"
#include "ql.h"

using namespace std;

PF_Manager pfm;
RM_Manager rmm(pfm);
IX_Manager ixm(pfm);
SM_Manager smm(rmm, ixm);
QL_Manager qlm(smm, ixm, rmm);


//
// main
//
int main(int argc, char *argv[])
{
    RBparse(pfm, smm, qlm);
    cout << "Bye.\n";
}
