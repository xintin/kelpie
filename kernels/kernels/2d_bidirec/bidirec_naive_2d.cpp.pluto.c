#include <math.h>
#define ceild(n,d)  ceil(((double)(n))/((double)(d)))
#define floord(n,d) floor(((double)(n))/((double)(d)))
#define max(x,y)    ((x) > (y)? (x) : (y))
#define min(x,y)    ((x) < (y)? (x) : (y))

#include <iostream>
#define N 100000

void bidirec_tiled_2d(int a[N][N]) {
  int t1, t2, t3;
 register int lbv, ubv;

if (N >= 2) {
  for (t1=1;t1<=N-1;t1++) {
    for (t2=0;t2<=floord(N-1,32);t2++) {
      lbv=32*t2;
      ubv=min(N-1,32*t2+31);

      for (t3=lbv;t3<=ubv;t3++) {
        a[t1][t3] += a[t1 - 1][t3] + a[t1 - 1][N - 1 - t3];;
      }
    }
  }
}
}

int main() {
    int a[N][N];

    // Initialize the array 'a' with some values
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            a[i][j] = i + j; // Example initialization
        }
    }

    // Call the function
    bidirec_naive_2d(a);

    // Print the updated array
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            std::cout << a[i][j] << "\t";
        }
        std::cout << std::endl;
    }

    return 0;
}
