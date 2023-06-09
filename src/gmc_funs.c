#include "gmc_funs.h"

matrix sqr_dist(matrix m) {
    // Compute sum of squared columns vector
    double * ssc = malloc(m.w * sizeof(double));
    for(uint i = 0; i < m.w; i++)
        ssc[i] = block_sum_col_sqr(m.data + i, m.h, m.w);

    // Compute multiplication by transpose (upper triangular only)
    matrix mt = new_matrix(m.w, m.w);
    cblas_dsyrk(CblasRowMajor, CblasUpper, CblasTrans, m.w, m.h, 1.0, m.data, m.w, 0.0, mt.data, m.w);

    // Compute final matrix
    matrix d = new_matrix(m.w, m.w);
    for(uint i = 0; i < m.w; i++) {
        for(uint j = 0; j < m.w; j++) {
            if(i == j) {
                d.data[j * d.w + i] = d.data[i * d.w + j] = 0.0;
                continue;
            }

            double mul = i < j ? mt.data[i * mt.w + j] : mt.data[j * mt.w + i];
            d.data[j * d.w + i] = d.data[i * d.w + j] = ssc[i] + ssc[j] - 2.0 * mul;
        }
    }

    free(ssc);
    free_matrix(mt);
    return d;
}

matrix update_u(matrix q) { // Height of q is m
    for(int j = 1; j < q.h; j++) {
        for(int i = 0; i < q.w; i++) q.data[i] += q.data[j * q.w + i];
    }

    double mean = 0.0;
    for(int i = 0; i < q.w; i++) mean += q.data[i];
    mean /= q.w;

    double vmin = INFINITY;
    for(int i = 0; i < q.w; i++) {
        q.data[i] = (q.data[i] - mean) / q.h + 1.0 / q.w;
        if(vmin > q.data[i]) vmin = q.data[i];
    }

    if(vmin >= 0.0) return q;

    double f = 1.0;
    double lambda_m = 0.0;
    double lambda_d = 0.0;
    for(int i = 0; i < q.w; i++) q.data[i] = -q.data[i];

    for(int ft = 1; ft <= 100 && (f < 0.0 ? -f : f) > 10e-10; ft++) {
        double sum = 0.0;
        int npos = 0;
        for(int i = 0; i < q.w; i++) {
            if((q.data[i] += lambda_d) > 0.0) {
                sum += q.data[i];
                npos++;
            } 
        }

        double g = (double)npos / q.w - 1.0 + EPS;
        f = sum / q.w - lambda_m;
        lambda_m += (lambda_d = -f / g);
    }

    for(int i = 0; i < q.w; i++) {
        if((q.data[i] = -q.data[i]) < 0.0) q.data[i] = 0.0;
    }

    q.h = 1;
    return q;
}

matrix update_f(matrix F, matrix U, double * ev, uint c) {
    for(int y = 0; y < U.w; y++) {
        for(int x = y; x < U.w; x++)
            F.data[y * U.w + x] = (U.data[y * U.w + x] + U.data[x * U.w + y]) / -2.0;
            
        F.data[y * U.w + y] -= block_sum(F.data + y * U.w + y + 1, U.w - y - 1) + block_sum_col(F.data + y, y, U.w);
    }

    // Eigenvalues go in ascending order inside ev, eigenvectors are returned inside F
    int found_eigenvalue_n;
    double * eigenvectors = malloc(sizeof(double) * F.w * (c + 1));
    int * ifail = malloc(sizeof(int) * (c + 1));

    // We are using row major upper triangular, but LAPACKE's dsyevx seems to have issues with row major,
    // so column major with lower triangular is equivalent in symmetric matrices
    int E = LAPACK_COL_MAJOR;
    LAPACKE_dsyevx(E, 'V', 'I', 'L', F.w, F.data, F.w, -1.0, -1.0, 1, c + 1, 0.0, &found_eigenvalue_n, ev, eigenvectors, F.w, ifail);

    // Ignore the (c+1)th eigenvector, we only need that for the eigenvalue
    memcpy(F.data, eigenvectors, sizeof(double) * F.w * c);
    F.h = c; 

    free(ifail);
    free(eigenvectors);
    return F;
}

int __find_comp(int * y, int i) {
    if(y[i] != i) y[i] = __find_comp(y, y[i]);
    return y[i];
}

void __merge_comp(int * y, int * ranks, int a, int b) {
    if(ranks[a] < ranks[b]) {
        y[a] = b;
        return;
    } 

    if(ranks[b] < ranks[a]) {
        y[b] = a;
        return;
    }

    y[b] = a;
    ranks[a]++;
}

int connected_comp(bool * adj, int * y, int num) {
    int * parents = malloc(num * sizeof(int));
    int * ranks = malloc(num * sizeof(int));
    memset(y, 0xFF, num * sizeof(int));

    for(int i = 0; i < num; i++) {
        parents[i] = i;
        ranks[i] = 0;
    }
    
    for(int j = 0; j < num; j++)
        for(int x = 0; x < j; x++)
            if(adj[j * num + x]) {
                int parentJ = __find_comp(parents, j);
                int parentX = __find_comp(parents, x);
                __merge_comp(parents, ranks, parentJ, parentX);
            } 

    int c = 0;
    for(int i = 0; i < num; i++) {
        int root = __find_comp(parents, i);
        if(y[root] == -1) y[root] = c++;
        y[i] = y[root];
    }

    free(parents);
    free(ranks);
    return c;
}
