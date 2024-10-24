#include <math.h>
#define ceild(n,d)  ceil(((double)(n))/((double)(d)))
#define floord(n,d) floor(((double)(n))/((double)(d)))
#define max(x,y)    ((x) > (y)? (x) : (y))
#define min(x,y)    ((x) < (y)? (x) : (y))

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

#define NSLIP 10068  // Define the size of the matrix

using namespace std;

using Matrix = vector<vector<double>>;

// Function to perform Cholesky decomposition naively
void cholesky_naive(Matrix& a, vector<double>& fdot, int nSlip) {
/* Copyright (C) 1991-2021 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */
  int t1, t2, t3, t4, t5, t6, t7;
 register int lbv, ubv;
/* Start of CLooG code */
if (nSlip >= 2) {
  for (t2=0;t2<=floord(nSlip-1,32);t2++) {
    lbv=max(1,32*t2);
    ubv=min(nSlip-1,32*t2+31);
#pragma ivdep
#pragma vector always
    for (t3=lbv;t3<=ubv;t3++) {
      fdot[t3] = 0.0;;
    }
  }
  for (t2=0;t2<=floord(nSlip-1,32);t2++) {
    for (t3=t2;t3<=min(floord(nSlip-1,16),floord(32*t2+nSlip+30,32));t3++) {
      if (16*t3 == nSlip-1) {
        if ((nSlip+15)%16 == 0) {
          a[(nSlip-1)][(nSlip-1)] = a[(nSlip-1)][(nSlip-1)] - fdot[(nSlip-1)];;
        }
      }
      for (t4=max(max(1,32*t2),32*t3-nSlip+1);t4<=min(32*t2+31,16*t3-1);t4++) {
        for (t5=32*t3;t5<=min(32*t3+31,t4+nSlip-1);t5++) {
          fdot[t4] = 0.0;;
          for (t6=0;t6<=t4-1;t6++) {
            fdot[t4] += a[t4][t6] * a[t6][(-t4+t5)];;
          }
          a[t4][(-t4+t5)] = a[t4][(-t4+t5)] - fdot[t4];;
          fdot[t4] = 0.0;;
          for (t6=t4;t6<=2*t4-1;t6++) {
            fdot[t4] += a[(-t4+t5)][(-t4+t6)] * a[(-t4+t6)][t4];;
          }
          a[(-t4+t5)][t4] = (a[(-t4+t5)][t4] - fdot[t4]) / a[t4][t4];;
        }
      }
      if ((t2 <= floord(t3,2)) && (t2 >= ceild(t3-1,2)) && (t3 >= 1) && (t3 <= floord(nSlip-2,16))) {
        a[16*t3][16*t3] = a[16*t3][16*t3] - fdot[16*t3];;
        for (t5=32*t3+1;t5<=min(32*t3+31,16*t3+nSlip-1);t5++) {
          fdot[16*t3] = 0.0;;
          for (t6=0;t6<=16*t3-1;t6++) {
            fdot[16*t3] += a[16*t3][t6] * a[t6][(-16*t3+t5)];;
          }
          a[16*t3][(-16*t3+t5)] = a[16*t3][(-16*t3+t5)] - fdot[16*t3];;
          fdot[16*t3] = 0.0;;
          for (t6=16*t3;t6<=32*t3-1;t6++) {
            fdot[16*t3] += a[(-16*t3+t5)][(-16*t3+t6)] * a[(-16*t3+t6)][16*t3];;
          }
          a[(-16*t3+t5)][16*t3] = (a[(-16*t3+t5)][16*t3] - fdot[16*t3]) / a[16*t3][16*t3];;
        }
      }
      for (t4=max(32*t2,16*t3+1);t4<=min(min(nSlip-2,32*t2+31),16*t3+15);t4++) {
        for (t5=max(32*t3,t4);t5<=2*t4-1;t5++) {
          fdot[t4] += a[t4][(-t4+t5)] * a[(-t4+t5)][t4];;
        }
        a[t4][t4] = a[t4][t4] - fdot[t4];;
        for (t5=2*t4+1;t5<=min(32*t3+31,t4+nSlip-1);t5++) {
          fdot[t4] = 0.0;;
          for (t6=0;t6<=t4-1;t6++) {
            fdot[t4] += a[t4][t6] * a[t6][(-t4+t5)];;
          }
          a[t4][(-t4+t5)] = a[t4][(-t4+t5)] - fdot[t4];;
          fdot[t4] = 0.0;;
          for (t6=t4;t6<=2*t4-1;t6++) {
            fdot[t4] += a[(-t4+t5)][(-t4+t6)] * a[(-t4+t6)][t4];;
          }
          a[(-t4+t5)][t4] = (a[(-t4+t5)][t4] - fdot[t4]) / a[t4][t4];;
        }
      }
      if ((t2 >= ceild(nSlip-32,32)) && (t3 <= floord(nSlip-2,16)) && (t3 >= ceild(nSlip-16,16))) {
        for (t5=max(32*t3,nSlip-1);t5<=2*nSlip-3;t5++) {
          fdot[(nSlip-1)] += a[(nSlip-1)][(t5-nSlip+1)] * a[(t5-nSlip+1)][(nSlip-1)];;
        }
        a[(nSlip-1)][(nSlip-1)] = a[(nSlip-1)][(nSlip-1)] - fdot[(nSlip-1)];;
      }
      for (t4=max(32*t2,16*t3+16);t4<=min(nSlip-1,32*t2+31);t4++) {
        for (t5=max(32*t3,t4);t5<=32*t3+31;t5++) {
          fdot[t4] += a[t4][(-t4+t5)] * a[(-t4+t5)][t4];;
        }
      }
    }
  }
}
/* End of CLooG code */
}

int main() {
    // Initialize random seed
    srand(static_cast<unsigned>(time(0)));

    // Initialize matrix 'a' with random values
    Matrix a(NSLIP, vector<double>(NSLIP, 0.0));
    for(int i = 0; i < NSLIP; i++) {
        for(int j = 0; j < NSLIP; j++) {
            a[i][j] = static_cast<double>(rand()) / RAND_MAX;
        }
    }

    // Initialize vector 'fdot'
    vector<double> fdot(NSLIP, 0.0);

    // Perform Cholesky decomposition naively
    cholesky_naive(a, fdot, NSLIP);

    // Optional: Output a confirmation message
    cout << "Cholesky decomposition completed successfully." << endl;

    return 0;
}
