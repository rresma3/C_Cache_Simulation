/* 
 * Ryan Resma rmr3429
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

#define BLOCK32 8 // for 32 x 32 matrices
#define BLOCK64 8 // for 64 x 64 matrice
#define BLOCK6167 17 // for 61 x 67 matrices

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    int x, y, row, column;
    int a, b, c, d, e, f, g, h; // temp variables

    if(M == 32 && N == 32) { // 32 x 32 matrix 
        // divide matrix into submatrixes of n x n
        // only do the diagonals since the submatrix
        // itself doesn't move if it's a diagonal
        for(x = 0; x < M; x += BLOCK32) {
            // place the transpose of each submatrix in A into B
            for(row = 0; row < BLOCK32; row++) {
                for(column = 0; column < BLOCK32; column++) {
                    // diagonal value, will evict itself so wait to store
                    if(row == column) {
                        b = A[x + row][x + column];
                    } else {
                        a = A[x + row][x + column];
                        B[x + column][x + row] = a;
                    } 
                }   
                B[x + row][x + row] = b; // diagonal 
            }
        }
    
        // transpose next
        for(y = 0; y < M; y += BLOCK32) {
            // diagonals are already done, so skip them
            for(x = 0; x < y; x += BLOCK32) {
                // preform transpose in each submatrix, but 
                // swap to the "transpose" (opposite) submatrix in B
                for(row = 0; row < BLOCK32; row++) {
                    for(column = 0; column < BLOCK32; column++) {
                        a = A[y + row][x + column];
                        B[x + column][y + row] = a;
                    }
                }
        
                for(row = 0; row < BLOCK32; row++) {
                    for(column = 0; column < BLOCK32; column++) {
                        a = A[x + column][y + row];
                        B[y + row][x + column] = a;
                    }
                }
            }
        }
    } else if(M == 64 && N == 64) { // 64 x 64 matrix
            // go through array as 8 x 8 submatrices
            for(y = 0; y < N; y += BLOCK64) {
                for(x = 0; x < M; x += BLOCK64) {

                    // intial elements in top right row of submatrix
                    // taken early to take advantage of the fact
                    // we open 8 integers every time we hit a new
                    // line.  However, since we only have 4 free variables,
                    // we can't save more numbers.
                    e = A[x][y + 4];
                    f = A[x][y + 5];
                    g = A[x][y + 6];
                    h = A[x][y + 7];
        
                    // repeat loop for every row in left side of submatrix
                    for (row = 0; row < BLOCK64; row++) { // used as counter
                        // get the values...
                        a = A[x + row][y];
                        b = A[x + row][y + 1];
                        c = A[x + row][y + 2];
                        d = A[x + row][y + 3];

                        // and store them.
                        B[y][x + row] = a;
                        B[y + 1][x + row] = b;
                        B[y + 2][x + row] = c;
                        B[y + 3][x + row] = d;
                    }

                    // go back up right side of submatrix to
                    // take advantage of the fact we
                    // can have 8 numbers in cache
                    for (row = 7; row > 0; row--) {
                        a = A[x + row][y + 4];
                        b = A[x + row][y + 5];
                        c = A[x + row][y + 6];
                        d = A[x + row][y + 7];
          
                        B[y + 4][x + row] = a;
                        B[y + 5][x + row] = b;
                        B[y + 6][x + row] = c;
                        B[y + 7][x + row] = d;
                    }

                    // clean up last elements
                    B[y + 4][x] = e;
                    B[y + 5][x] = f;
                    B[y + 6][x] = g;
                    B[y + 7][x] = h;
                }
            } 
    } else { // 61 x 67 matrix     
        // preform meta transpose next
        for(x = 0; x < M; x += BLOCK6167) {
            for(y = 0; y < N; y += BLOCK6167) {
                // preform transpose in each submatrix, but 
                // swap to the "transpose" (opposite) submatrix in B
                for(row = 0; row < BLOCK6167; row++) {
                    for(column = 0; column < BLOCK6167; column++) {
                        if(y + row < N && x + column < M) {
                            a = A[y + row][x + column];
                            B[x + column][y + row] = a;
                        }
                    }
                }
            }
        }
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
