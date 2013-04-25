#include "radiant.h"
#include "math3d_collision.h"

using namespace radiant;
using namespace radiant::math3d;

//===============================================================================
// @ IvCovariance.cpp
// 
// Helper routines for computing properties of covariance matrices
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include "covariance.h"

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ compute_covariance_matrix()
//-------------------------------------------------------------------------------
// Computes and returns the real, symmetric covariance matrix, along with the
// mean vector.
//-------------------------------------------------------------------------------
void math3d::compute_covariance_matrix(matrix3& C, point3& mean,
                                       const point3* points, unsigned int num_points)
{
    ASSERT(num_points > 1);

    mean = point3::origin;

    // compute the mean (the centroid of the points)
    unsigned int i;
    for (i = 0; i < num_points; i++)
        mean += points[i];

    mean /= (float)num_points;

    // compute the (co)variances
    float var_x = 0.0f;
    float var_y = 0.0f;
    float var_z = 0.0f;
    float cov_xy = 0.0f;
    float cov_xz = 0.0f;
    float cov_yz = 0.0f;

    for (i = 0; i < num_points; i++)
    {
        vector3 diff = vector3(points[i] - mean);
        
        var_x += diff.x * diff.x;
        var_y += diff.y * diff.y;
        var_z += diff.z * diff.z;
        cov_xy += diff.x * diff.y;
        cov_xz += diff.x * diff.z;
        cov_yz += diff.y * diff.z;
    }

    // divide all of the (co)variances by n - 1 
    // (skipping the division if n = 2, since that would be division by 1.0
    if (num_points > 2)
    {
        const float normalize = (float)(num_points - 1);
        var_x /= normalize;
        var_y /= normalize;
        var_z /= normalize;
        cov_xy /= normalize;
        cov_xz /= normalize;
        cov_yz /= normalize;
    }

    // pack values into the covariance matrix, which is symmetric
    C(0,0) = var_x;
    C(1,1) = var_y;
    C(2,2) = var_z;
    C(1,0) = C(0,1) = cov_xy;
    C(2,0) = C(0,2) = cov_xz;
    C(1,2) = C(2,1) = cov_yz;

}  // End of compute_covariance_matrix()


//-------------------------------------------------------------------------------
// @ symmetric_householder_3x3()
//-------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------
static void symmetric_householder_3x3 (const matrix3& M, matrix3& Q, 
                                       vector3& diag, vector3& subd)
{
    // Computes the Householder reduction of M, computing:
    //
    // T = Q^t M Q
    //
    //   Input:   
    //     symmetric 3x3 matrix M
    //   Output:  
    //     orthogonal matrix Q
    //     diag, diagonal entries of T
    //     subd, lower-triangle entries of T (T is symmetric)

    // T will be stored as follows (because it is symmetric and tridiagonal):
    //
    // | diag[0]  subd[0]  0       |
    // | subd[0]  diag[1]  subd[1] |
    // | 0        subd[1]  diag[2] |

    diag[0] = M(0,0); // in both cases below, T(0,0) = M(0,0) 
    subd[2] = 0; // T is to be a tri-diagonal matrix - the (2,0) and (0,2) 
                 // entries must be zero, so we don't need subd[2] for this step

    // so we know that T will actually be:
    //
    // | M(0,0)   subd[0]  0       |
    // | subd[0]  diag[1]  subd[1] |
    // | 0        subd[1]  diag[2] |

    // so the only question remaining is the lower-right block and subd[0]

    if (math3d::abs(M(2,0)) < k_epsilon)
    {
        // if M(2,0) (and thus M(0,2)) is zero, then the matrix is already in
        // tridiagonal form.  As such, the Q matix is the identity, and we
        // just extract the diagonal and subdiagonal components of T as-is
        Q.set_identity();

        // so we see that T will actually be:
        //
        // | M(0,0)  M(1,0)  0      |
        // | M(1,0)  M(1,1)  M(2,1) |
        // | 0       M(2,1)  M(2,2) |
        diag[1] = M(1,1);
        diag[2] = M(2,2);

        subd[0] = M(1,0);
        subd[1] = M(2,1);
    }
    else
    {
        // grab the lower triangle of the matrix, minus a, which we don't need
        // (see above)
        // |       |
        // | b d   |
        // | c e f |
        const float b = M(1,0);
        const float c = M(2,0);
        const float d = M(1,1);
        const float e = M(2,1);
        const float f = M(2,2);

        // normalize b and c to unit length and store in u and v
        // we want the lower-right block we create to be orthonormal
        const float L = math3d::sqrt(b * b + c * c);
        const float u = b / L;
        const float v = c / L;
        Q.set_rows(vector3(1.0f, 0.0f, 0.0f),
                   vector3(0.0f, u,    v),
                   vector3(0.0f, v,    -u));

        float q = 2 * u * e + v * (f - d);
        diag[1] = d + v * q;
        diag[2] = f - v * q;

        subd[0] = L;
        subd[1] = e - u * q;

        // so we see that T will actually be:
        //
        // | M(0,0)  L       0     |
        // | L       d+c*q   e-b*q |
        // | 0       e-b*q   f-c*q |
    }
}  // End of symmetric_householder_3x3

//-------------------------------------------------------------------------------
// @ ql_algorithm()
//-------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------
static int ql_algorithm (matrix3& M, vector3& diag, vector3& subd)
{
    // QL iteration with implicit shifting to reduce matrix from tridiagonal
    // to diagonal

    int L;
    for (L = 0; L < 3; L++)
    {
        // As this is an iterative process, we need to keep a maximum number of
        // iterations, just in case something is wrong - we cannot afford to
        // loop forever
        const int max_iterations = 32;

        int iter;
        for (iter = 0; iter < max_iterations; iter++)
        {
            int m;
            for (m = L; m <= 1; m++)
            {
                float dd = math3d::abs(diag[m]) + math3d::abs(diag[m+1]);
                if (math3d::abs(subd[m]) + dd == dd)
                    break;
            }
            if (m == L)
                break;

            float g = (diag[L+1]-diag[L])/(2*subd[L]);
            float r = math3d::sqrt(g*g+1);
            if (g < 0)
                g = diag[m]-diag[L]+subd[L]/(g-r);
            else
                g = diag[m]-diag[L]+subd[L]/(g+r);
            float s = 1, c = 1, p = 0;
            for (int i = m-1; i >= L; i--)
            {
                float f = s*subd[i], b = c*subd[i];
                if (math3d::abs(f) >= math3d::abs(g))
                {
                    c = g/f;
                    r = math3d::sqrt(c*c+1);
                    subd[i+1] = f*r;
                    c *= (s = 1/r);
                }
                else
                {
                    s = f/g;
                    r = math3d::sqrt(s*s+1);
                    subd[i+1] = g*r;
                    s *= (c = 1/r);
                }
                g = diag[i+1]-p;
                r = (diag[i]-g)*s+2*b*c;
                p = s*r;
                diag[i+1] = g+p;
                g = c*r-b;

                for (int k = 0; k < 3; k++)
                {
                    f = M(k,i+1);
                    M(k,i+1) = s*M(k,i)+c*f;
                    M(k,i) = c*M(k,i)-s*f;
                }
            }
            diag[L] -= p;
            subd[L] = g;
            subd[m] = 0;
        }

        // exceptional case - matrix took more iterations than should be 
        // possible to move to diagonal form
        if (iter == max_iterations)
            return 0;
    }

    return 1;
}  // End of ql_algorithm

//-------------------------------------------------------------------------------
// @ get_real_systemmetric_eigenvectors()
//-------------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------------
void radiant::math3d::get_real_systemmetric_eigenvectors(vector3& v1, vector3& v2, vector3& v3, 
                                                         const matrix3& A)
{    
    vector3 eigenvalues;
    vector3 lowerTriangle;
    matrix3 Q;

    symmetric_householder_3x3(A, Q, eigenvalues, lowerTriangle);
    ql_algorithm(Q, eigenvalues, lowerTriangle);

    // Sort the eigenvalues from greatest to smallest, and use these indices
    // to sort the eigenvectors
    int v1Index, v2Index, v3Index;
    if (eigenvalues[0] > eigenvalues[1])
    {
        if (eigenvalues[1] > eigenvalues[2])
        {
            v1Index = 0;
            v2Index = 1;
            v3Index = 2;
        }
        else if (eigenvalues[2] > eigenvalues[0])
        {
            v1Index = 2;
            v2Index = 0;
            v3Index = 1;
        }
        else
        {
            v1Index = 0;
            v2Index = 2;
            v3Index = 1;
        }
    }
    else // eigenvalues[1] >= eigenvalues[0]
    {
        if (eigenvalues[0] > eigenvalues[2])
        {
            v1Index = 1;
            v2Index = 0;
            v3Index = 2;
        }
        else if (eigenvalues[2] > eigenvalues[1])
        {
            v1Index = 2;
            v2Index = 1;
            v3Index = 0;
        }
        else
        {
            v1Index = 1;
            v2Index = 2;
            v3Index = 0;
        }
    }

    // Sort the eigenvectors into the output vectors
    v1 = Q.get_column(v1Index);
    v2 = Q.get_column(v2Index);
    v3 = Q.get_column(v3Index);

    // If the resulting eigenvectors are left-handed, negate the 
    // min-eigenvalue eigenvector to make it right-handed
    if (v1.dot(v2.cross(v3)) < 0.0f)
        v3 = -v3;
}  // End of get_real_systemmetric_eigenvectors

