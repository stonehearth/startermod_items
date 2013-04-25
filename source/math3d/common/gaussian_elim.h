//===============================================================================
// @ gaussian_elim.h
// 
// Gaussian elimination routines
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
// Routines to solve linear equations, get inverse, and determinants for
// matrices of arbitrary size
//
// Note that the matrices need to be in column major order, like the base matrices
// in the library
//
//===============================================================================

#ifndef _RADIANT_MATH_GAUSSIAN_ELIM_H
#define _RADIANT_MATH_GAUSSIAN_ELIM_H

//-------------------------------------------------------------------------------
//-- Function Prototypes --------------------------------------------------------
//-------------------------------------------------------------------------------

namespace radiant {
   namespace math3d {
      // solve for system Ax = b for n by n matrix and n-vector
      // will destroy contents of A and return result in b
      bool solve(float* b, float* A, unsigned int n);

      // invert matrix n by n matrix A
      // will store results in A
      bool invert_matrix(float* A, unsigned int n);


      // Get determinant of matrix using Gaussian elimination
      // Will not destroy A
      float determinant(float* A, unsigned int n);

   };
};

#endif
