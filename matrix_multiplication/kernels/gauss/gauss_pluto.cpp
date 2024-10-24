#include <math.h>
#define ceild(n,d)  ceil(((double)(n))/((double)(d)))
#define floord(n,d) floor(((double)(n))/((double)(d)))
#define max(x,y)    ((x) > (y)? (x) : (y))
#define min(x,y)    ((x) < (y)? (x) : (y))

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <tuple>
#include <cfloat>
#include <cmath>
#include <chrono>

using namespace std;

// Define constants
constexpr int M = 1024; // Width of the image
constexpr int N = 768;  // Height of the image
constexpr int T = 1;    // To translate k (Set to 1 for a 3-element kernel)

// Function to apply 2D Gaussian blur
void apply_gaussian_blur(
    const vector<vector<int>>& in_image,
    vector<vector<int>>& gauss_image,
    const vector<int>& Gauss,
    const vector<int>& tot
) {
    // Temporary images and accumulators
    vector<vector<int>> g_tmp_image(N, vector<int>(M, 0));
    vector<vector<vector<int>>> g_acc1(N, vector<vector<int>>(M, vector<int>(4, 0)));
    vector<vector<vector<int>>> g_acc2(N, vector<vector<int>>(M, vector<int>(4, 0)));

    int gauss_index, in_x, tmp_y, x, y, k;
    
  int t1, t2, t3, t4, t5, t6, t7, t8;
  int lbv, ubv;

  if ((M >= 1) && (N >= 4)) {
    if (M >= 3) {
      for (t3=0;t3<=floord(N-3,32);t3++) {
        for (t4=0;t4<=floord(M-2,32);t4++) {
          for (t5=max(1,32*t3);t5<=min(N-3,32*t3+31);t5++) {
            lbv=max(1,32*t4);
            ubv=min(M-2,32*t4+31);

            for (t6=lbv;t6<=ubv;t6++) {
              g_acc2[t5][t6][0] = 0;;
            }
          }
        }
      }
    }
    for (t3=1;t3<=N-3;t3++) {
      for (t4=0;t4<=floord(M-1,32);t4++) {
        for (t5=2*t4;t5<=min(floord(M,16),2*t4+2);t5++) {
          for (t6=32*t5;t6<=min(min(2*M,64*t4+64),32*t5+31);t6++) {
            for (t7=max(ceild(t6-2,2),32*t4);t7<=min(min(floord(t6,2),M-1),32*t4+31);t7++) {
              gauss_index = (t6-2*t7) - T + 1;;
              in_x = t3 + (t6-2*t7);;
            }
          }
        }
      }
    }
    if (M >= 3) {
      for (t3=1;t3<=N-3;t3++) {
        for (t4=0;t4<=floord(M-2,32);t4++) {
          for (t5=2*t4;t5<=min(floord(M-1,16),2*t4+2);t5++) {
            for (t6=max(2,32*t5);t6<=min(min(2*M-2,64*t4+64),32*t5+31);t6++) {
              for (t7=max(max(1,ceild(t6-2,2)),32*t4);t7<=min(min(floord(t6,2),M-2),32*t4+31);t7++) {
                gauss_index = (t6-2*t7) - T + 1;;
              }
            }
          }
        }
      }
    }
    for (t3=0;t3<=floord(N-3,32);t3++) {
      for (t4=0;t4<=floord(M-1,32);t4++) {
        for (t5=max(1,32*t3);t5<=min(N-3,32*t3+31);t5++) {
          lbv=32*t4;
          ubv=min(M-1,32*t4+31);

          for (t6=lbv;t6<=ubv;t6++) {
            g_acc1[t5][t6][0] = 0;;
          }
        }
      }
    }
    for (t3=0;t3<=floord(N-3,32);t3++) {
      for (t4=0;t4<=floord(M-1,32);t4++) {
        for (t6=max(1,32*t3);t6<=min(N-3,32*t3+31);t6++) {
          for (t7=32*t4;t7<=min(M-1,32*t4+31);t7++) {
            for (t8=0;t8<=2;t8++) {
              g_acc1[t6][t7][t8 + 2 - T] = g_acc1[t6][t7][t8 + 1 - T] + in_image[t6+t8][t7] * Gauss[t8 - T + 1];;
            }
          }
        }
      }
    }
    for (t3=0;t3<=floord(N-3,32);t3++) {
      for (t4=0;t4<=floord(M-1,32);t4++) {
        for (t5=max(1,32*t3);t5<=min(N-3,32*t3+31);t5++) {
          lbv=32*t4;
          ubv=min(M-1,32*t4+31);

          for (t6=lbv;t6<=ubv;t6++) {
            g_tmp_image[t5][t6] = g_acc1[t5][t6][3] / tot[3];;
          }
        }
      }
    }
    if (M >= 3) {
      for (t3=0;t3<=floord(N-3,32);t3++) {
        for (t4=0;t4<=floord(M-2,32);t4++) {
          for (t6=max(1,32*t3);t6<=min(N-3,32*t3+31);t6++) {
            for (t7=max(1,32*t4);t7<=min(M-2,32*t4+31);t7++) {
              for (t8=0;t8<=2;t8++) {
                g_acc2[t6][t7][t8 + 2 - T] = g_acc2[t6][t7][t8 + 1 - T] + g_tmp_image[t6][t7 + t8 - T] * Gauss[t8 - T + 1];;
              }
            }
          }
        }
      }
    }
    if (M >= 3) {
      for (t3=0;t3<=floord(N-3,32);t3++) {
        for (t4=0;t4<=floord(M-2,32);t4++) {
          for (t5=max(1,32*t3);t5<=min(N-3,32*t3+31);t5++) {
            lbv=max(1,32*t4);
            ubv=min(M-2,32*t4+31);

            for (t6=lbv;t6<=ubv;t6++) {
              gauss_image[t5][t6] = g_acc2[t5][t6][3] / tot[3];;
            }
          }
        }
      }
    }
  }
}

int main(int argc, char* argv[]) {
    // Seed for reproducibility
    srand(static_cast<unsigned>(time(0)));

    // Initialize input and output images
    vector<vector<int>> in_image(N, vector<int>(M, 0));      // Input image
    vector<vector<int>> gauss_image(N, vector<int>(M, 0));   // Output image

    // Initialize Gaussian kernel with padding for Gauss[-1]
    // Gauss[0] = Gauss[-1], Gauss[1] = Gauss[0], Gauss[2] = Gauss[1], Gauss[3] = Gauss[2]
    vector<int> Gauss = {0, 1, 2, 1}; // Example kernel: [Gauss[-1], Gauss[0], Gauss[1], Gauss[2]]

    // Initialize tot array
    // tot[0] = 0
    // tot[1] = tot[0] + Gauss[1] = 0 + 1 = 1
    // tot[2] = tot[1] + Gauss[2] = 1 + 2 = 3
    // tot[3] = tot[2] + Gauss[3] = 3 + 1 = 4
    vector<int> tot(4, 0);
    tot[0] = 0;
    for(int k = 0; k <= 2; k++) { // k from 0 to 2
        int index = k + 1;        // 1,2,3
        int prev_index = k;        // 0,1,2
        if(index >= static_cast<int>(tot.size()) || prev_index >= static_cast<int>(tot.size())) {
            cerr << "Error: tot indexing out of bounds during initialization.\n";
            exit(EXIT_FAILURE);
        }
        tot[index] = tot[prev_index] + Gauss[k +1];
    }

    // Initialize in_image with sample data (for demonstration)
    for(int x = 0; x < N; x++) {
        for(int y = 0; y < M; y++) {
            in_image[x][y] = rand() % 256; // Example: random pixel values between 0 and 255
        }
    }

    // Apply Gaussian blur
    apply_gaussian_blur(in_image, gauss_image, Gauss, tot);


    for(int x = 0; x < min(N, 10); x++) {
        for(int y = 0; y < min(M, 10); y++) {
            cout << gauss_image[x][y] << " ";
        }
        cout << endl;
    }


    return 0;
}
