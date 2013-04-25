#include "radiant.h"
#include "common.h"
#include "radiant.pb.h"

using namespace radiant;
using namespace radiant::math3d;

//===============================================================================
// @ matrix3.cpp
// 
// 3x3 matrix class
// ------------------------------------------------------------------------------
// Copyright (C) 2008 by Elsevier, Inc. All rights reserved.
//
//===============================================================================

//-------------------------------------------------------------------------------
//-- Dependencies ---------------------------------------------------------------
//-------------------------------------------------------------------------------

#include "matrix3.h"
#include "quaternion.h"
#include "vector3.h"

//-------------------------------------------------------------------------------
//-- Static Members -------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ matrix3::matrix3()
//-------------------------------------------------------------------------------
// Axis-angle constructor
//-------------------------------------------------------------------------------
matrix3::matrix3(const quaternion& quat)
{
    (void) rotation(quat);

}   // End of matrix3::matrix3()


//-------------------------------------------------------------------------------
// @ matrix3::matrix3()
//-------------------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------------------
matrix3::matrix3(const matrix3& other)
{
    v[0] = other.v[0];
    v[1] = other.v[1];
    v[2] = other.v[2];
    v[3] = other.v[3];
    v[4] = other.v[4];
    v[5] = other.v[5];
    v[6] = other.v[6];
    v[7] = other.v[7];
    v[8] = other.v[8];

}   // End of matrix3::matrix3()


//-------------------------------------------------------------------------------
// @ matrix3::operator=()
//-------------------------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------------------
matrix3&
matrix3::operator=(const matrix3& other)
{
    // if same object
    if (this == &other)
        return *this;
        
    v[0] = other.v[0];
    v[1] = other.v[1];
    v[2] = other.v[2];
    v[3] = other.v[3];
    v[4] = other.v[4];
    v[5] = other.v[5];
    v[6] = other.v[6];
    v[7] = other.v[7];
    v[8] = other.v[8];
    
    return *this;

}   // End of matrix3::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Text output for debugging
//-------------------------------------------------------------------------------
std::ostream& 
math3d::operator<<(std::ostream& out, const matrix3& source)
{
    // row
    for (unsigned int i = 0; i < 3; ++i)
    {
        out << "| ";
        // column
        for (unsigned int j = 0; j < 3; ++j)
        {
            out << source.v[ j*3 + i ] << ' ';
        }
        out << '|' << std::endl;
    }

    return out;
    
}   // End of operator<<()


//-------------------------------------------------------------------------------
// @ matrix3::operator==()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
matrix3::operator==(const matrix3& other) const
{
    for (unsigned int i = 0; i < 9; ++i)
    {
        if (!math3d::are_equal(v[i], other.v[i]))
            return false;
    }
    return true;

}   // End of matrix3::operator==()


//-------------------------------------------------------------------------------
// @ matrix3::operator!=()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
matrix3::operator!=(const matrix3& other) const
{
    for (unsigned int i = 0; i < 9; ++i)
    {
        if (!math3d::are_equal(v[i], other.v[i]))
            return true;
    }
    return false;

}   // End of matrix3::operator!=()


//-------------------------------------------------------------------------------
// @ matrix3::is_zero()
//-------------------------------------------------------------------------------
// Check for zero matrix
//-------------------------------------------------------------------------------
bool 
matrix3::is_zero() const
{
    for (unsigned int i = 0; i < 9; ++i)
    {
        if (!math3d::is_zero(v[i]))
            return false;
    }
    return true;

}   // End of matrix3::is_zero()


//-------------------------------------------------------------------------------
// @ matrix3::is_identity()
//-------------------------------------------------------------------------------
// Check for identity matrix
//-------------------------------------------------------------------------------
bool 
matrix3::is_identity() const
{
    return ::math3d::are_equal(1.0f, v[0])
        && ::math3d::are_equal(1.0f, v[4])
        && ::math3d::are_equal(1.0f, v[8])
        && math3d::is_zero(v[1]) 
        && math3d::is_zero(v[2])
        && math3d::is_zero(v[3])
        && math3d::is_zero(v[5]) 
        && math3d::is_zero(v[6])
        && math3d::is_zero(v[7]);

}   // End of matrix3::is_identity()


//-------------------------------------------------------------------------------
// @ matrix3::set_rows()
//-------------------------------------------------------------------------------
// Set matrix, row by row
//-------------------------------------------------------------------------------
void 
matrix3::set_rows(const vector3& row1, const vector3& row2, const vector3& row3)
{
    v[0] = row1.x;
    v[3] = row1.y;
    v[6] = row1.z;

    v[1] = row2.x;
    v[4] = row2.y;
    v[7] = row2.z;

    v[2] = row3.x;
    v[5] = row3.y;
    v[8] = row3.z;

}   // End of matrix3::set_rows()


//-------------------------------------------------------------------------------
// @ matrix3::get_rows()
//-------------------------------------------------------------------------------
// Get matrix, row by row
//-------------------------------------------------------------------------------
void 
matrix3::get_rows(vector3& row1, vector3& row2, vector3& row3) const
{
    row1.x = v[0];
    row1.y = v[3];
    row1.z = v[6];

    row2.x = v[1];
    row2.y = v[4];
    row2.z = v[7];

    row3.x = v[2];
    row3.y = v[5];
    row3.z = v[8];
}   // End of matrix3::get_rows()


//-------------------------------------------------------------------------------
// @ matrix3::GetRow()
//-------------------------------------------------------------------------------
// Get matrix, row by row
//-------------------------------------------------------------------------------
vector3 
matrix3::GetRow(unsigned int i) const
{
    ASSERT(i < 3);
    return vector3(v[i], v[i+3], v[i+6]);

}   // End of matrix3::GetRow()


//-------------------------------------------------------------------------------
// @ matrix3::set_columns()
//-------------------------------------------------------------------------------
// Set matrix, row by row
//-------------------------------------------------------------------------------
void 
matrix3::set_columns(const vector3& col1, const vector3& col2, const vector3& col3)
{
    v[0] = col1.x;
    v[1] = col1.y;
    v[2] = col1.z;

    v[3] = col2.x;
    v[4] = col2.y;
    v[5] = col2.z;

    v[6] = col3.x;
    v[7] = col3.y;
    v[8] = col3.z;

}   // End of matrix3::set_columns()


//-------------------------------------------------------------------------------
// @ matrix3::get_columns()
//-------------------------------------------------------------------------------
// Get matrix, row by row
//-------------------------------------------------------------------------------
void 
matrix3::get_columns(vector3& col1, vector3& col2, vector3& col3) const
{
    col1.x = v[0];
    col1.y = v[1];
    col1.z = v[2];

    col2.x = v[3];
    col2.y = v[4];
    col2.z = v[5];

    col3.x = v[6];
    col3.y = v[7];
    col3.z = v[8];

}   // End of matrix3::get_columns()


//-------------------------------------------------------------------------------
// @ matrix3::get_columns()
//-------------------------------------------------------------------------------
// Get matrix, row by row
//-------------------------------------------------------------------------------
vector3 
matrix3::get_column(unsigned int i) const 
{
    ASSERT(i < 3);
    return vector3(v[3*i], v[3*i+1], v[3*i+2]);

}   // End of matrix3::get_columns()


//-------------------------------------------------------------------------------
// @ matrix3::clean()
//-------------------------------------------------------------------------------
// Set elements close to zero equal to zero
//-------------------------------------------------------------------------------
void
matrix3::clean()
{
    for (unsigned int i = 0; i < 9; ++i)
    {
        if (math3d::is_zero(v[i]))
            v[i] = 0.0f;
    }

}   // End of matrix3::clean()


//-------------------------------------------------------------------------------
// @ matrix3::identity()
//-------------------------------------------------------------------------------
// Set to identity matrix
//-------------------------------------------------------------------------------
void
matrix3::set_identity()
{
    v[0] = 1.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
    v[3] = 0.0f;
    v[4] = 1.0f;
    v[5] = 0.0f;
    v[6] = 0.0f;
    v[7] = 0.0f;
    v[8] = 1.0f;

}   // End of matrix3::identity()


//-------------------------------------------------------------------------------
// @ matrix3::inverse()
//-------------------------------------------------------------------------------
// Set self to inverse
//-------------------------------------------------------------------------------
matrix3& 
matrix3::inverse()
{
    *this = math3d::inverse(*this);

    return *this;

}   // End of matrix3::inverse()


//-------------------------------------------------------------------------------
// @ matrix3::inverse()
//-------------------------------------------------------------------------------
// Compute matrix inverse
//-------------------------------------------------------------------------------
matrix3 
math3d::inverse(const matrix3& mat)
{
    matrix3 result;
    
    // compute determinant
    float cofactor0 = mat.v[4]*mat.v[8] - mat.v[5]*mat.v[7];
    float cofactor3 = mat.v[2]*mat.v[7] - mat.v[1]*mat.v[8];
    float cofactor6 = mat.v[1]*mat.v[5] - mat.v[2]*mat.v[4];
    float det = mat.v[0]*cofactor0 + mat.v[3]*cofactor3 + mat.v[6]*cofactor6;
    if (math3d::is_zero(det))
    {
        ASSERT(false);
        ERROR_OUT("Matrix33::inverse() -- singular matrix\n");
        return result;
    }

    // create adjoint matrix and multiply by 1/det to get inverse
    float invDet = 1.0f/det;
    result.v[0] = invDet*cofactor0;
    result.v[1] = invDet*cofactor3;
    result.v[2] = invDet*cofactor6;
   
    result.v[3] = invDet*(mat.v[5]*mat.v[6] - mat.v[3]*mat.v[8]);
    result.v[4] = invDet*(mat.v[0]*mat.v[8] - mat.v[2]*mat.v[6]);
    result.v[5] = invDet*(mat.v[2]*mat.v[3] - mat.v[0]*mat.v[5]);

    result.v[6] = invDet*(mat.v[3]*mat.v[7] - mat.v[4]*mat.v[6]);
    result.v[7] = invDet*(mat.v[1]*mat.v[6] - mat.v[0]*mat.v[7]);
    result.v[8] = invDet*(mat.v[0]*mat.v[4] - mat.v[1]*mat.v[3]);

    return result;

}   // End of matrix3::inverse()


//-------------------------------------------------------------------------------
// @ matrix3::transpose()
//-------------------------------------------------------------------------------
// Set self to transpose
//-------------------------------------------------------------------------------
matrix3& 
matrix3::transpose()
{
    float temp = v[1];
    v[1] = v[3];
    v[3] = temp;

    temp = v[2];
    v[2] = v[6];
    v[6] = temp;

    temp = v[5];
    v[5] = v[7];
    v[7] = temp;

    return *this;

}   // End of matrix3::transpose()


//-------------------------------------------------------------------------------
// @ matrix3::transpose()
//-------------------------------------------------------------------------------
// Compute matrix transpose
//-------------------------------------------------------------------------------
matrix3 
math3d::transpose(const matrix3& mat)
{
    matrix3 result;

    result.v[0] = mat.v[0];
    result.v[1] = mat.v[3];
    result.v[2] = mat.v[6];
    result.v[3] = mat.v[1];
    result.v[4] = mat.v[4];
    result.v[5] = mat.v[7];
    result.v[6] = mat.v[2];
    result.v[7] = mat.v[5];
    result.v[8] = mat.v[8];

    return result;

}   // End of matrix3::transpose()


//-------------------------------------------------------------------------------
// @ matrix3::determinant()
//-------------------------------------------------------------------------------
// Get determinant of matrix
//-------------------------------------------------------------------------------
float 
matrix3::determinant() const
{
    return v[0]*(v[4]*v[8] - v[5]*v[7]) 
         + v[3]*(v[2]*v[7] - v[1]*v[8])
         + v[6]*(v[1]*v[5] - v[2]*v[4]);

}   // End of matrix3::determinant()


//-------------------------------------------------------------------------------
// @ matrix3::Adjoint()
//-------------------------------------------------------------------------------
// Compute matrix adjoint
//-------------------------------------------------------------------------------
matrix3 
matrix3::Adjoint() const
{
    matrix3 result;
    
    // compute transpose of cofactors
    result.v[0] = v[4]*v[8] - v[5]*v[7];
    result.v[1] = v[2]*v[7] - v[1]*v[8];
    result.v[2] = v[1]*v[5] - v[2]*v[4];
   
    result.v[3] = v[5]*v[6] - v[3]*v[8];
    result.v[4] = v[0]*v[8] - v[2]*v[6];
    result.v[5] = v[2]*v[3] - v[0]*v[5];

    result.v[6] = v[3]*v[7] - v[4]*v[6];
    result.v[7] = v[1]*v[6] - v[0]*v[7];
    result.v[8] = v[0]*v[4] - v[1]*v[3];

    return result;

}   // End of matrix3::Adjoint()


//-------------------------------------------------------------------------------
// @ matrix3::Guard()
//-------------------------------------------------------------------------------
// Get trace of matrix
//-------------------------------------------------------------------------------
float 
matrix3::Guard() const
{
    return v[0] + v[4] + v[8];

}   // End of matrix3::Guard()


//-------------------------------------------------------------------------------
// @ matrix3::rotation()
//-------------------------------------------------------------------------------
// Set as rotation matrix based on quaternion
//-------------------------------------------------------------------------------
matrix3& 
matrix3::rotation(const quaternion& rotate)
{
    ASSERT(rotate.is_unit());

    float xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;

    xs = rotate.x+rotate.x;   
    ys = rotate.y+rotate.y;
    zs = rotate.z+rotate.z;
    wx = rotate.w*xs;
    wy = rotate.w*ys;
    wz = rotate.w*zs;
    xx = rotate.x*xs;
    xy = rotate.x*ys;
    xz = rotate.x*zs;
    yy = rotate.y*ys;
    yz = rotate.y*zs;
    zz = rotate.z*zs;

    v[0] = 1.0f - (yy + zz);
    v[3] = xy - wz;
    v[6] = xz + wy;
    
    v[1] = xy + wz;
    v[4] = 1.0f - (xx + zz);
    v[7] = yz - wx;
    
    v[2] = xz - wy;
    v[5] = yz + wx;
    v[8] = 1.0f - (xx + yy);

    return *this;

}   // End of rotation()


//----------------------------------------------------------------------------
// @ matrix3::rotation()
// ---------------------------------------------------------------------------
// Sets the matrix to a rotation matrix (by Euler angles).
//----------------------------------------------------------------------------
matrix3 &
matrix3::rotation(float z_rotation, float y_rotation, float x_rotation)
{
    // This is an "unrolled" contatenation of rotation matrices X Y & Z
    float Cx, Sx;
    math3d::sincos(x_rotation, Sx, Cx);

    float Cy, Sy;
    math3d::sincos(y_rotation, Sy, Cy);

    float Cz, Sz;
    math3d::sincos(z_rotation, Sz, Cz);

    v[0] =  (Cy * Cz);
    v[3] = -(Cy * Sz);  
    v[6] =  Sy;

    v[1] =  (Sx * Sy * Cz) + (Cx * Sz);
    v[4] = -(Sx * Sy * Sz) + (Cx * Cz);
    v[7] = -(Sx * Cy); 

    v[2] = -(Cx * Sy * Cz) + (Sx * Sz);
    v[5] =  (Cx * Sy * Sz) + (Sx * Cz);
    v[8] =  (Cx * Cy);

    return *this;

}  // End of matrix3::rotation()


//----------------------------------------------------------------------------
// @ matrix3::rotation()
// ---------------------------------------------------------------------------
// Sets the matrix to a rotation matrix (by axis and angle).
//----------------------------------------------------------------------------
matrix3 &
matrix3::rotation(const vector3& axis, float angle)
{
    float c, s;
    math3d::sincos(angle, s, c);
    float t = 1.0f - c;

    vector3 nAxis = axis;
    nAxis.normalize();

    // intermediate values
    float tx = t*nAxis.x;  float ty = t*nAxis.y;  float tz = t*nAxis.z;
    float sx = s*nAxis.x;  float sy = s*nAxis.y;  float sz = s*nAxis.z;
    float txy = tx*nAxis.y; float tyz = tx*nAxis.z; float txz = tx*nAxis.z;

    // set matrix
    v[0] = tx*nAxis.x + c;
    v[3] = txy - sz;
    v[6] = txz + sy;
    v[1] = txy + sz;
    v[4] = ty*nAxis.y + c;
    v[7] = tyz - sx;
    v[2] = txz - sy;
    v[5] = tyz + sx;
    v[8] = tz*nAxis.z + c;

    return *this;

}  // End of matrix3::rotation()


//-------------------------------------------------------------------------------
// @ matrix3::scaling()
//-------------------------------------------------------------------------------
// Set as scaling matrix based on vector
//-------------------------------------------------------------------------------
matrix3& 
matrix3::scaling(const vector3& scaleFactors)
{
    v[0] = scaleFactors.x;
    v[1] = 0.0f;
    v[2] = 0.0f;
    v[3] = 0.0f;
    v[4] = scaleFactors.y;
    v[5] = 0.0f;
    v[6] = 0.0f;
    v[7] = 0.0f;
    v[8] = scaleFactors.z;

    return *this;

}   // End of scaling()


//-------------------------------------------------------------------------------
// @ matrix3::rotation_x()
//-------------------------------------------------------------------------------
// Set as rotation matrix, rotating by 'angle' radians around x-axis
//-------------------------------------------------------------------------------
matrix3& 
matrix3::rotation_x(float angle)
{
    float sintheta, costheta;
    math3d::sincos(angle, sintheta, costheta);

    v[0] = 1.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
    v[3] = 0.0f;
    v[4] = costheta;
    v[5] = sintheta;
    v[6] = 0.0f;
    v[7] = -sintheta;
    v[8] = costheta;

    return *this;

}   // End of rotation_x()


//-------------------------------------------------------------------------------
// @ matrix3::rotation_y()
//-------------------------------------------------------------------------------
// Set as rotation matrix, rotating by 'angle' radians around y-axis
//-------------------------------------------------------------------------------
matrix3& 
matrix3::rotation_y(float angle)
{
    float sintheta, costheta;
    math3d::sincos(angle, sintheta, costheta);

    v[0] = costheta;
    v[1] = 0.0f;
    v[2] = -sintheta;
    v[3] = 0.0f;
    v[4] = 1.0f;
    v[5] = 0.0f;
    v[6] = sintheta;
    v[7] = 0.0f;
    v[8] = costheta;

    return *this;

}   // End of rotation_y()


//-------------------------------------------------------------------------------
// @ matrix3::rotation_z()
//-------------------------------------------------------------------------------
// Set as rotation matrix, rotating by 'angle' radians around z-axis
//-------------------------------------------------------------------------------
matrix3& 
matrix3::rotation_z(float angle)
{
    float sintheta, costheta;
    math3d::sincos(angle, sintheta, costheta);

    v[0] = costheta;
    v[1] = sintheta;
    v[2] = 0.0f;
    v[3] = -sintheta;
    v[4] = costheta;
    v[5] = 0.0f;
    v[6] = 0.0f;
    v[7] = 0.0f;
    v[8] = 1.0f;

    return *this;

}   // End of rotation_z()


//----------------------------------------------------------------------------
// @ matrix3::get_fixed_angles()
// ---------------------------------------------------------------------------
// Gets one set of possible z-y-x fixed angles that will generate this matrix
// Assumes that this is a rotation matrix
//----------------------------------------------------------------------------
void
matrix3::get_fixed_angles(float& z_rotation, float& y_rotation, float& x_rotation)
{
    float Cx, Sx;
    float Cy, Sy;
    float Cz, Sz;

    Sy = v[6];
    Cy = math3d::sqrt(1.0f - Sy*Sy);
    // normal case
    if (!math3d::is_zero(Cy))
    {
        float factor = 1.0f/Cy;
        Sx = -v[7]*factor;
        Cx = v[8]*factor;
        Sz = -v[3]*factor;
        Cz = v[0]*factor;
    }
    // x and z axes aligned
    else
    {
        Sz = 0.0f;
        Cz = 1.0f;
        Sx = v[5];
        Cx = v[4];
    }

    z_rotation = atan2f(Sz, Cz);
    y_rotation = atan2f(Sy, Cy);
    x_rotation = atan2f(Sx, Cx);

}  // End of matrix3::get_fixed_angles()


//----------------------------------------------------------------------------
// @ matrix3::get_axis_angle()
// ---------------------------------------------------------------------------
// Gets one possible axis-angle pair that will generate this matrix
// Assumes that this is a rotation matrix
//----------------------------------------------------------------------------
void
matrix3::get_axis_angle(vector3& axis, float& angle)
{
    float trace = v[0] + v[4] + v[8];
    float cosTheta = 0.5f*(trace - 1.0f);
    angle = acosf(cosTheta);

    // angle is zero, axis can be anything
    if (math3d::is_zero(angle))
    {
        axis = vector3::unit_x;
    }
    // standard case
    else if (angle < k_pi-k_epsilon)
    {
        axis.set(v[5]-v[7], v[6]-v[2], v[1]-v[3]);
        axis.normalize();
    }
    // angle is 180 degrees
    else
    {
        unsigned int i = 0;
        if (v[4] > v[0])
            i = 1;
        if (v[8] > v[i + 3*i])
            i = 2;
        unsigned int j = (i+1)%3;
        unsigned int k = (j+1)%3;
        float s = math3d::sqrt(v[i + 3*i] - v[j + 3*j] - v[k + 3*k] + 1.0f);
        axis[i] = 0.5f*s;
        float recip = 1.0f/s;
        axis[j] = (v[i + 3*j])*recip;
        axis[k] = (v[k + 3*i])*recip;
    }

}  // End of matrix3::get_axis_angle()


//-------------------------------------------------------------------------------
// @ matrix3::operator+()
//-------------------------------------------------------------------------------
// Matrix addition 
//-------------------------------------------------------------------------------
matrix3
matrix3::operator+(const matrix3& other) const
{
    matrix3 result;

    for (unsigned int i = 0; i < 9; ++i)
    {
        result.v[i] = v[i] + other.v[i];
    }

    return result;

}   // End of matrix3::operator-()


//-------------------------------------------------------------------------------
// @ matrix3::operator+=()
//-------------------------------------------------------------------------------
// Matrix addition by self
//-------------------------------------------------------------------------------
matrix3&
matrix3::operator+=(const matrix3& other)
{
    for (unsigned int i = 0; i < 9; ++i)
    {
        v[i] += other.v[i];
    }

    return *this;

}   // End of matrix3::operator+=()


//-------------------------------------------------------------------------------
// @ matrix3::operator-()
//-------------------------------------------------------------------------------
// Matrix subtraction 
//-------------------------------------------------------------------------------
matrix3
matrix3::operator-(const matrix3& other) const
{
    matrix3 result;

    for (unsigned int i = 0; i < 9; ++i)
    {
        result.v[i] = v[i] - other.v[i];
    }

    return result;

}   // End of matrix3::operator-()


//-------------------------------------------------------------------------------
// @ matrix3::operator-=()
//-------------------------------------------------------------------------------
// Matrix subtraction by self
//-------------------------------------------------------------------------------
matrix3&
matrix3::operator-=(const matrix3& other)
{
    for (unsigned int i = 0; i < 9; ++i)
    {
        v[i] -= other.v[i];
    }

    return *this;

}   // End of matrix3::operator-=()


//-------------------------------------------------------------------------------
// @ matrix3::operator-=() (unary)
//-------------------------------------------------------------------------------
// Negate self and return
//-------------------------------------------------------------------------------
matrix3
matrix3::operator-() const
{
    matrix3 result;

    for (unsigned int i = 0; i < 16; ++i)
    {
        result.v[i] = -v[i];
    }

    return result;

}    // End of quaternion::operator-()


//-------------------------------------------------------------------------------
// @ matrix3::operator*()
//-------------------------------------------------------------------------------
// Matrix multiplication
//-------------------------------------------------------------------------------
matrix3  
matrix3::operator*(const matrix3& other) const
{
    matrix3 result;

    result.v[0] = v[0]*other.v[0] + v[3]*other.v[1] + v[6]*other.v[2];
    result.v[1] = v[1]*other.v[0] + v[4]*other.v[1] + v[7]*other.v[2];
    result.v[2] = v[2]*other.v[0] + v[5]*other.v[1] + v[8]*other.v[2];
    result.v[3] = v[0]*other.v[3] + v[3]*other.v[4] + v[6]*other.v[5];
    result.v[4] = v[1]*other.v[3] + v[4]*other.v[4] + v[7]*other.v[5];
    result.v[5] = v[2]*other.v[3] + v[5]*other.v[4] + v[8]*other.v[5];
    result.v[6] = v[0]*other.v[6] + v[3]*other.v[7] + v[6]*other.v[8];
    result.v[7] = v[1]*other.v[6] + v[4]*other.v[7] + v[7]*other.v[8];
    result.v[8] = v[2]*other.v[6] + v[5]*other.v[7] + v[8]*other.v[8];

    return result;

}   // End of matrix3::operator*()


//-------------------------------------------------------------------------------
// @ matrix3::operator*=()
//-------------------------------------------------------------------------------
// Matrix multiplication by self
//-------------------------------------------------------------------------------
matrix3&
matrix3::operator*=(const matrix3& other)
{
    matrix3 result;

    result.v[0] = v[0]*other.v[0] + v[3]*other.v[1] + v[6]*other.v[2];
    result.v[1] = v[1]*other.v[0] + v[4]*other.v[1] + v[7]*other.v[2];
    result.v[2] = v[2]*other.v[0] + v[5]*other.v[1] + v[8]*other.v[2];
    result.v[3] = v[0]*other.v[3] + v[3]*other.v[4] + v[6]*other.v[5];
    result.v[4] = v[1]*other.v[3] + v[4]*other.v[4] + v[7]*other.v[5];
    result.v[5] = v[2]*other.v[3] + v[5]*other.v[4] + v[8]*other.v[5];
    result.v[6] = v[0]*other.v[6] + v[3]*other.v[7] + v[6]*other.v[8];
    result.v[7] = v[1]*other.v[6] + v[4]*other.v[7] + v[7]*other.v[8];
    result.v[8] = v[2]*other.v[6] + v[5]*other.v[7] + v[8]*other.v[8];

    for (unsigned int i = 0; i < 9; ++i)
    {
        v[i] = result.v[i];
    }

    return *this;

}   // End of matrix3::operator*=()


//-------------------------------------------------------------------------------
// @ matrix3::operator*()
//-------------------------------------------------------------------------------
// Matrix-column vector multiplication
//-------------------------------------------------------------------------------
vector3   
matrix3::operator*(const vector3& other) const
{
    vector3 result;

    result.x = v[0]*other.x + v[3]*other.y + v[6]*other.z;
    result.y = v[1]*other.x + v[4]*other.y + v[7]*other.z;
    result.z = v[2]*other.x + v[5]*other.y + v[8]*other.z;

    return result;

}   // End of matrix3::operator*()

//-------------------------------------------------------------------------------
// @ matrix3::operator*()
//-------------------------------------------------------------------------------
// Matrix-column point multiplication
//-------------------------------------------------------------------------------
point3   
matrix3::operator*(const point3& other) const
{
    point3 result;

    result.x = v[0]*other.x + v[3]*other.y + v[6]*other.z;
    result.y = v[1]*other.x + v[4]*other.y + v[7]*other.z;
    result.z = v[2]*other.x + v[5]*other.y + v[8]*other.z;

    return result;

}   // End of matrix3::operator*()


//-------------------------------------------------------------------------------
// @ matrix3::operator*()
//-------------------------------------------------------------------------------
// Row vector-matrix multiplication
//-------------------------------------------------------------------------------
vector3   
math3d::operator*(const vector3& vector, const matrix3& mat)
{
    vector3 result;

    result.x = mat.v[0]*vector.x + mat.v[1]*vector.y + mat.v[2]*vector.z;
    result.y = mat.v[3]*vector.x + mat.v[4]*vector.y + mat.v[5]*vector.z;
    result.z = mat.v[6]*vector.x + mat.v[7]*vector.y + mat.v[8]*vector.z;

    return result;

}   // End of matrix3::operator*()

//-------------------------------------------------------------------------------
// @ matrix3::operator*()
//-------------------------------------------------------------------------------
// Row point-matrix multiplication
//-------------------------------------------------------------------------------
point3   
math3d::operator*(const point3& point, const matrix3& mat)
{
    point3 result;

    result.x = mat.v[0]*point.x + mat.v[1]*point.y + mat.v[2]*point.z;
    result.y = mat.v[3]*point.x + mat.v[4]*point.y + mat.v[5]*point.z;
    result.z = mat.v[6]*point.x + mat.v[7]*point.y + mat.v[8]*point.z;

    return result;

}   // End of matrix3::operator*()


//-------------------------------------------------------------------------------
// @ matrix3::*=()
//-------------------------------------------------------------------------------
// Matrix-scalar multiplication
//-------------------------------------------------------------------------------
matrix3& matrix3::operator*=(float scalar)
{
    v[0] *= scalar;
    v[1] *= scalar;
    v[2] *= scalar;
    v[3] *= scalar;
    v[4] *= scalar;
    v[5] *= scalar;
    v[6] *= scalar;
    v[7] *= scalar;
    v[8] *= scalar;

    return *this;
}  // End of matrix3::operator*=()


//-------------------------------------------------------------------------------
// @ friend matrix3 *()
//-------------------------------------------------------------------------------
// Matrix-scalar multiplication
//-------------------------------------------------------------------------------
matrix3 operator*(float scalar, const matrix3& matrix)
{
    matrix3 result;
    result.v[0] = matrix.v[0] * scalar;
    result.v[1] = matrix.v[1] * scalar;
    result.v[2] = matrix.v[2] * scalar;
    result.v[3] = matrix.v[3] * scalar;
    result.v[4] = matrix.v[4] * scalar;
    result.v[5] = matrix.v[5] * scalar;
    result.v[6] = matrix.v[6] * scalar;
    result.v[7] = matrix.v[7] * scalar;
    result.v[8] = matrix.v[8] * scalar;

    return result;
}  // End of friend matrix3 operator*()


//-------------------------------------------------------------------------------
// @ matrix3::*()
//-------------------------------------------------------------------------------
// Matrix-scalar multiplication
//-------------------------------------------------------------------------------
matrix3 matrix3::operator*(float scalar) const
{
    matrix3 result;
    result.v[0] = v[0] * scalar;
    result.v[1] = v[1] * scalar;
    result.v[2] = v[2] * scalar;
    result.v[3] = v[3] * scalar;
    result.v[4] = v[4] * scalar;
    result.v[5] = v[5] * scalar;
    result.v[6] = v[6] * scalar;
    result.v[7] = v[7] * scalar;
    result.v[8] = v[8] * scalar;

    return result;
}  // End of matrix3::operator*=()


