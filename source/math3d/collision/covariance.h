//===============================================================================
// @ IvCovariance.h
// 
// Helper routines for computing properties of covariance matrices
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//
//
//===============================================================================

#ifndef _RADIANT_MATH3D_COLLISION_COVARIANCE_H
#define _RADIANT_MATH3D_COLLISION_COVARIANCE_H

namespace radiant {
   namespace math3d {
      void compute_covariance_matrix(matrix3& C, point3& mean,
                                     const point3* points, unsigned int num_points);
      void get_real_systemmetric_eigenvectors(vector3& v1, vector3& v2, vector3& v3, 
                                             const matrix3& A);
   }
};

#endif
