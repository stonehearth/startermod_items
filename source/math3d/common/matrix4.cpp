#include "radiant.h"
#include "common.h"
#include "radiant.pb.h"

using namespace radiant;
using namespace radiant::math3d;

//===============================================================================
// @ matrix4.cpp
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
#include "matrix4.h"
#include "quaternion.h"
#include "vector3.h"
#include "vector4.h"


//-------------------------------------------------------------------------------
//-- Static Members -------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ matrix4::matrix4()
//-------------------------------------------------------------------------------
// Axis-angle constructor
//-------------------------------------------------------------------------------
matrix4::matrix4(const quaternion& quat)
{
    (void) rotation(quat);

}   // End of matrix4::matrix4()


//-------------------------------------------------------------------------------
// @ matrix4::matrix4()
//-------------------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------------------
matrix4::matrix4(const matrix4& other)
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
    v[9] = other.v[9];
    v[10] = other.v[10];
    v[11] = other.v[11];
    v[12] = other.v[12];
    v[13] = other.v[13];
    v[14] = other.v[14];
    v[15] = other.v[15];

}   // End of matrix4::matrix4()

//-------------------------------------------------------------------------------
// @ matrix4::matrix4()
//-------------------------------------------------------------------------------
// matrix3 conversion constructor
//-------------------------------------------------------------------------------
matrix4::matrix4(const matrix3& other)
{
    (void) rotation(other);

}   // End of matrix4::matrix4()


//-------------------------------------------------------------------------------
// @ matrix4::operator=()
//-------------------------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------------------
matrix4&
matrix4::operator=(const matrix4& other)
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
    v[9] = other.v[9];
    v[10] = other.v[10];
    v[11] = other.v[11];
    v[12] = other.v[12];
    v[13] = other.v[13];
    v[14] = other.v[14];
    v[15] = other.v[15];
    
    return *this;

}   // End of matrix4::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Text output for debugging
//-------------------------------------------------------------------------------
std::ostream& 
math3d::operator<<(std::ostream& out, const matrix4& source)
{
    // row
    for (unsigned int i = 0; i < 4; ++i)
    {
        out << "| ";
        // column
        for (unsigned int j = 0; j < 4; ++j)
        {
            out << source.v[ j*4 + i ] << ' ';
        }
        out << '|' << std::endl;
    }

    return out;
    
}   // End of operator<<()
    

//-------------------------------------------------------------------------------
// @ matrix4::operator==()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
matrix4::operator==(const matrix4& other) const
{
    for (unsigned int i = 0; i < 16; ++i)
    {
        if (!math3d::are_equal(v[i], other.v[i]))
            return false;
    }
    return true;

}   // End of matrix4::operator==()


//-------------------------------------------------------------------------------
// @ matrix4::operator!=()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
matrix4::operator!=(const matrix4& other) const
{
    for (unsigned int i = 0; i < 16; ++i)
    {
        if (!math3d::are_equal(v[i], other.v[i]))
            return true;
    }
    return false;

}   // End of matrix4::operator!=()


//-------------------------------------------------------------------------------
// @ matrix4::is_zero()
//-------------------------------------------------------------------------------
// Check for zero matrix
//-------------------------------------------------------------------------------
bool 
matrix4::is_zero() const
{
    for (unsigned int i = 0; i < 16; ++i)
    {
        if (!math3d::is_zero(v[i]))
            return false;
    }
    return true;

}   // End of matrix4::is_zero()


//-------------------------------------------------------------------------------
// @ matrix4::is_identity()
//-------------------------------------------------------------------------------
// Check for identity matrix
//-------------------------------------------------------------------------------
bool 
matrix4::is_identity() const
{
    return ::math3d::are_equal(1.0f, v[0])
        && ::math3d::are_equal(1.0f, v[5])
        && ::math3d::are_equal(1.0f, v[10])
        && ::math3d::are_equal(1.0f, v[15])
        && math3d::is_zero(v[1]) 
        && math3d::is_zero(v[2])
        && math3d::is_zero(v[3])
        && math3d::is_zero(v[4]) 
        && math3d::is_zero(v[6])
        && math3d::is_zero(v[7])
        && math3d::is_zero(v[8])
        && math3d::is_zero(v[9])
        && math3d::is_zero(v[11])
        && math3d::is_zero(v[12])
        && math3d::is_zero(v[13])
        && math3d::is_zero(v[14]);

}   // End of matrix4::is_identity()


//-------------------------------------------------------------------------------
// @ matrix4::clean()
//-------------------------------------------------------------------------------
// Set elements close to zero equal to zero
//-------------------------------------------------------------------------------
void
matrix4::clean()
{
    for (unsigned int i = 0; i < 16; ++i)
    {
        if (math3d::is_zero(v[i]))
            v[i] = 0.0f;
    }

}   // End of matrix4::clean()


//-------------------------------------------------------------------------------
// @ matrix4::identity()
//-------------------------------------------------------------------------------
// Set to identity matrix
//-------------------------------------------------------------------------------
void
matrix4::identity()
{
    v[0] = 1.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
    v[3] = 0.0f;
    v[4] = 0.0f;
    v[5] = 1.0f;
    v[6] = 0.0f;
    v[7] = 0.0f;
    v[8] = 0.0f;
    v[9] = 0.0f;
    v[10] = 1.0f;
    v[11] = 0.0f;
    v[12] = 0.0f;
    v[13] = 0.0f;
    v[14] = 0.0f;
    v[15] = 1.0f;

}   // End of matrix4::identity()


//-----------------------------------------------------------------------------
// @ matrix4::affine_inverse()
//-----------------------------------------------------------------------------
/// Set self to matrix inverse, assuming a standard affine matrix 
/// (bottom row is 0 0 0 1)
//-----------------------------------------------------------------------------
matrix4& 
matrix4::affine_inverse()
{
    *this = math3d::affine_inverse(*this);    
    return *this;

}   // End of matrix4::affine_inverse()


//-----------------------------------------------------------------------------
// @ affine_inverse()
//-----------------------------------------------------------------------------
/// Compute matrix inverse, assuming a standard affine matrix 
/// (bottom row is 0 0 0 1)
//-----------------------------------------------------------------------------
static matrix4 
math3d::affine_inverse(const matrix4& mat)
{
    matrix4 result;
    
    // compute upper left 3x3 matrix determinant
    float cofactor0 = mat.v[5]*mat.v[10] - mat.v[6]*mat.v[9];
    float cofactor4 = mat.v[2]*mat.v[9] - mat.v[1]*mat.v[10];
    float cofactor8 = mat.v[1]*mat.v[6] - mat.v[2]*mat.v[5];
    float det = mat.v[0]*cofactor0 + mat.v[4]*cofactor4 + mat.v[8]*cofactor8;
    if (math3d::is_zero(det))
    {
        ASSERT(false);
        ERROR_OUT("Matrix44::inverse() -- singular matrix\n");
        return result;
    }

    // create adjunct matrix and multiply by 1/det to get upper 3x3
    float invDet = 1.0f/det;
    result.v[0] = invDet*cofactor0;
    result.v[1] = invDet*cofactor4;
    result.v[2] = invDet*cofactor8;
   
    result.v[4] = invDet*(mat.v[6]*mat.v[8] - mat.v[4]*mat.v[10]);
    result.v[5] = invDet*(mat.v[0]*mat.v[10] - mat.v[2]*mat.v[8]);
    result.v[6] = invDet*(mat.v[2]*mat.v[4] - mat.v[0]*mat.v[6]);

    result.v[8] = invDet*(mat.v[4]*mat.v[9] - mat.v[5]*mat.v[8]);
    result.v[9] = invDet*(mat.v[1]*mat.v[8] - mat.v[0]*mat.v[9]);
    result.v[10] = invDet*(mat.v[0]*mat.v[5] - mat.v[1]*mat.v[4]);

    // multiply -translation by inverted 3x3 to get its inverse
    
    result.v[12] = -result.v[0]*mat.v[12] - result.v[4]*mat.v[13] - result.v[8]*mat.v[14];
    result.v[13] = -result.v[1]*mat.v[12] - result.v[5]*mat.v[13] - result.v[9]*mat.v[14];
    result.v[14] = -result.v[2]*mat.v[12] - result.v[6]*mat.v[13] - result.v[10]*mat.v[14];

    return result;

}   // End of affine_inverse()


//-------------------------------------------------------------------------------
// @ matrix4::transpose()
//-------------------------------------------------------------------------------
// Set self to transpose
//-------------------------------------------------------------------------------
matrix4& 
matrix4::transpose()
{
    float temp = v[1];
    v[1] = v[4];
    v[4] = temp;

    temp = v[2];
    v[2] = v[8];
    v[8] = temp;

    temp = v[3];
    v[2] = v[12];
    v[12] = temp;

    temp = v[6];
    v[6] = v[9];
    v[9] = temp;

    temp = v[7];
    v[7] = v[13];
    v[13] = temp;

    temp = v[11];
    v[11] = v[14];
    v[14] = temp;

    return *this;

}   // End of matrix4::transpose()


//-------------------------------------------------------------------------------
// @ transpose()
//-------------------------------------------------------------------------------
// Compute matrix transpose
//-------------------------------------------------------------------------------
matrix4 
transpose(const matrix4& mat)
{
    matrix4 result;

    result.v[0] = mat.v[0];
    result.v[1] = mat.v[4];
    result.v[2] = mat.v[8];
    result.v[3] = mat.v[12];
    result.v[4] = mat.v[1];
    result.v[5] = mat.v[5];
    result.v[6] = mat.v[9];
    result.v[7] = mat.v[13];
    result.v[8] = mat.v[2];
    result.v[9] = mat.v[6];
    result.v[10] = mat.v[10];
    result.v[11] = mat.v[14];
    result.v[12] = mat.v[3];
    result.v[13] = mat.v[7];
    result.v[14] = mat.v[11];
    result.v[15] = mat.v[15];

    return result;

}   // End of transpose()


//-------------------------------------------------------------------------------
// @ matrix4::translation()
//-------------------------------------------------------------------------------
// Set as translation matrix based on vector
//-------------------------------------------------------------------------------
matrix4& 
matrix4::translation(const vector3& xlate)
{
    v[0] = 1.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
    v[3] = 0.0f;
    v[4] = 0.0f;
    v[5] = 1.0f;
    v[6] = 0.0f;
    v[7] = 0.0f;
    v[8] = 0.0f;
    v[9] = 0.0f;
    v[10] = 1.0f;
    v[11] = 0.0f;
    v[12] = xlate.x;
    v[13] = xlate.y;
    v[14] = xlate.z;
    v[15] = 1.0f;

    return *this;

}   // End of matrix4::translation()


//-------------------------------------------------------------------------------
// @ matrix4::rotation()
//-------------------------------------------------------------------------------
// Set as rotation matrix based on quaternion
//-------------------------------------------------------------------------------
matrix4& 
matrix4::rotation(const quaternion& rotate)
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
    v[4] = xy - wz;
    v[8] = xz + wy;
    v[12] = 0.0f;
    
    v[1] = xy + wz;
    v[5] = 1.0f - (xx + zz);
    v[9] = yz - wx;
    v[13] = 0.0f;
    
    v[2] = xz - wy;
    v[6] = yz + wx;
    v[10] = 1.0f - (xx + yy);
    v[14] = 0.0f;

    v[3] = 0.0f;
    v[7] = 0.0f;
    v[11] = 0.0f;
    v[15] = 1.0f;

    return *this;

}   // End of rotation()


//-------------------------------------------------------------------------------
// @ matrix4::rotation()
//-------------------------------------------------------------------------------
// Set to rotation matrix based on 3x3 matrix (which better be a rotation)
//-------------------------------------------------------------------------------
matrix4&
matrix4::rotation(const matrix3& other)
{
    v[0] = other.v[0];
    v[1] = other.v[1];
    v[2] = other.v[2];
    v[3] = 0;
    v[4] = other.v[3];
    v[5] = other.v[4];
    v[6] = other.v[5];
    v[7] = 0;
    v[8] = other.v[6];
    v[9] = other.v[7];
    v[10] = other.v[8];
    v[11] = 0;
    v[12] = 0;
    v[13] = 0;
    v[14] = 0;
    v[15] = 1;

    return *this;

}   // End of matrix4::matrix4()


//----------------------------------------------------------------------------
// @ matrix4::rotation()
// ---------------------------------------------------------------------------
// Sets the matrix to a rotation matrix (by Euler angles).
//----------------------------------------------------------------------------
matrix4 &
matrix4::rotation(float z_rotation, float y_rotation, float x_rotation)
{
    // This is an "unrolled" contatenation of rotation matrices X Y & Z
    float Cx, Sx;
    math3d::sincos(x_rotation, Sx, Cx);

    float Cy, Sy;
    math3d::sincos(y_rotation, Sy, Cy);

    float Cz, Sz;
    math3d::sincos(z_rotation, Sz, Cz);

    v[0] =  (Cy * Cz);
    v[4] = -(Cy * Sz);  
    v[8] =  Sy;
    v[12] = 0.0f;

    v[1] =  (Sx * Sy * Cz) + (Cx * Sz);
    v[5] = -(Sx * Sy * Sz) + (Cx * Cz);
    v[9] = -(Sx * Cy); 
    v[13] = 0.0f;

    v[2] = -(Cx * Sy * Cz) + (Sx * Sz);
    v[6] =  (Cx * Sy * Sz) + (Sx * Cz);
    v[10] =  (Cx * Cy);
    v[14] = 0.0f;

    v[3] = 0.0f;
    v[7] = 0.0f;
    v[11] = 0.0f;
    v[15] = 1.0f;

    return *this;

}  // End of matrix4::rotation()


//----------------------------------------------------------------------------
// @ matrix4::rotation()
// ---------------------------------------------------------------------------
// Sets the matrix to a rotation matrix (by axis and angle).
//----------------------------------------------------------------------------
matrix4 &
matrix4::rotation(const vector3& axis, float angle)
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
    v[4] = txy - sz; 
    v[8] = txz + sy;
    v[12] = 0.0f;

    v[1] = txy + sz;
    v[5] = ty*nAxis.y + c;
    v[9] = tyz - sx;
    v[13] = 0.0f;

    v[2] = txz - sy;
    v[6] = tyz + sx;
    v[10] = tz*nAxis.z + c;
    v[14] = 0.0f;

    v[3] = 0.0f;
    v[7] = 0.0f;
    v[11] = 0.0f;
    v[15] = 1.0f;

    return *this;

}  // End of matrix4::rotation()


//-------------------------------------------------------------------------------
// @ matrix4::scaling()
//-------------------------------------------------------------------------------
// Set as scaling matrix based on vector
//-------------------------------------------------------------------------------
matrix4& 
matrix4::scaling(const vector3& scaleFactors)
{
    v[0] = scaleFactors.x;
    v[1] = 0.0f;
    v[2] = 0.0f;
    v[3] = 0.0f;
    v[4] = 0.0f;
    v[5] = scaleFactors.y;
    v[6] = 0.0f;
    v[7] = 0.0f;
    v[8] = 0.0f;
    v[9] = 0.0f;
    v[10] = scaleFactors.z;
    v[11] = 0.0f;
    v[12] = 0.0f;
    v[13] = 0.0f;
    v[14] = 0.0f;
    v[15] = 1.0f;

    return *this;

}   // End of scaling()


//-------------------------------------------------------------------------------
// @ matrix4::rotation_x()
//-------------------------------------------------------------------------------
// Set as rotation matrix, rotating by 'angle' radians around x-axis
//-------------------------------------------------------------------------------
matrix4& 
matrix4::rotation_x(float angle)
{
    float sintheta, costheta;
    math3d::sincos(angle, sintheta, costheta);

    v[0] = 1.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
    v[3] = 0.0f;
    v[4] = 0.0f;
    v[5] = costheta;
    v[6] = sintheta;
    v[7] = 0.0f;
    v[8] = 0.0f;
    v[9] = -sintheta;
    v[10] = costheta;
    v[11] = 0.0f;
    v[12] = 0.0f;
    v[13] = 0.0f;
    v[14] = 0.0f;
    v[15] = 1.0f;

    return *this;

}   // End of rotation_x()


//-------------------------------------------------------------------------------
// @ matrix4::rotation_y()
//-------------------------------------------------------------------------------
// Set as rotation matrix, rotating by 'angle' radians around y-axis
//-------------------------------------------------------------------------------
matrix4& 
matrix4::rotation_y(float angle)
{
    float sintheta, costheta;
    math3d::sincos(angle, sintheta, costheta);

    v[0] = costheta;
    v[1] = 0.0f;
    v[2] = -sintheta;
    v[3] = 0.0f;
    v[4] = 0.0f;
    v[5] = 1.0f;
    v[6] = 0.0f;
    v[7] = 0.0f;
    v[8] = sintheta;
    v[9] = 0.0f;
    v[10] = costheta;
    v[11] = 0.0f;
    v[12] = 0.0f;
    v[13] = 0.0f;
    v[14] = 0.0f;
    v[15] = 1.0f;    

    return *this;

}   // End of rotation_y()


//-------------------------------------------------------------------------------
// @ matrix4::rotation_z()
//-------------------------------------------------------------------------------
// Set as rotation matrix, rotating by 'angle' radians around z-axis
//-------------------------------------------------------------------------------
matrix4& 
matrix4::rotation_z(float angle)
{
    float sintheta, costheta;
    math3d::sincos(angle, sintheta, costheta);

    v[0] = costheta;
    v[1] = sintheta;
    v[2] = 0.0f;
    v[3] = 0.0f;
    v[4] = -sintheta;
    v[5] = costheta;
    v[6] = 0.0f;
    v[7] = 0.0f;
    v[8] = 0.0f;
    v[9] = 0.0f;
    v[10] = 1.0f;
    v[11] = 0.0f;
    v[12] = 0.0f;
    v[13] = 0.0f;
    v[14] = 0.0f;
    v[15] = 1.0f;

    return *this;

}   // End of rotation_z()


//----------------------------------------------------------------------------
// @ matrix4::get_fixed_angles()
// ---------------------------------------------------------------------------
// Gets one set of possible z-y-x fixed angles that will generate this matrix
// Assumes that upper 3x3 is a rotation matrix
//----------------------------------------------------------------------------
void
matrix4::get_fixed_angles(float& z_rotation, float& y_rotation, float& x_rotation)
{
    float Cx, Sx;
    float Cy, Sy;
    float Cz, Sz;

    Sy = v[8];
    Cy = math3d::sqrt(1.0f - Sy*Sy);
    // normal case
    if (!math3d::is_zero(Cy))
    {
        float factor = 1.0f/Cy;
        Sx = -v[9]*factor;
        Cx = v[10]*factor;
        Sz = -v[4]*factor;
        Cz = v[0]*factor;
    }
    // x and z axes aligned
    else
    {
        Sz = 0.0f;
        Cz = 1.0f;
        Sx = v[6];
        Cx = v[5];
    }

    z_rotation = atan2f(Sz, Cz);
    y_rotation = atan2f(Sy, Cy);
    x_rotation = atan2f(Sx, Cx);

}  // End of matrix4::get_fixed_angles()


//----------------------------------------------------------------------------
// @ matrix4::get_axis_angle()
// ---------------------------------------------------------------------------
// Gets one possible axis-angle pair that will generate this matrix
// Assumes that upper 3x3 is a rotation matrix
//----------------------------------------------------------------------------
void
matrix4::get_axis_angle(vector3& axis, float& angle)
{
    float trace = v[0] + v[5] + v[10];
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
        axis.set(v[6]-v[9], v[8]-v[2], v[1]-v[4]);
        axis.normalize();
    }
    // angle is 180 degrees
    else
    {
        unsigned int i = 0;
        if (v[5] > v[0])
            i = 1;
        if (v[10] > v[i + 4*i])
            i = 2;
        unsigned int j = (i+1)%3;
        unsigned int k = (j+1)%3;
        float s = math3d::sqrt(v[i + 4*i] - v[j + 4*j] - v[k + 4*k] + 1.0f);
        axis[i] = 0.5f*s;
        float recip = 1.0f/s;
        axis[j] = (v[i + 4*j])*recip;
        axis[k] = (v[k + 4*i])*recip;
    }

}  // End of matrix4::get_axis_angle()


//-------------------------------------------------------------------------------
// @ matrix4::operator+()
//-------------------------------------------------------------------------------
// Matrix addition 
//-------------------------------------------------------------------------------
matrix4
matrix4::operator+(const matrix4& other) const
{
    matrix4 result;

    for (unsigned int i = 0; i < 16; ++i)
    {
        result.v[i] = v[i] + other.v[i];
    }

    return result;

}   // End of matrix4::operator-()


//-------------------------------------------------------------------------------
// @ matrix4::operator+=()
//-------------------------------------------------------------------------------
// Matrix addition by self
//-------------------------------------------------------------------------------
matrix4&
matrix4::operator+=(const matrix4& other)
{
    for (unsigned int i = 0; i < 16; ++i)
    {
        v[i] += other.v[i];
    }

    return *this;

}   // End of matrix4::operator+=()


//-------------------------------------------------------------------------------
// @ matrix4::operator-()
//-------------------------------------------------------------------------------
// Matrix subtraction 
//-------------------------------------------------------------------------------
matrix4
matrix4::operator-(const matrix4& other) const
{
    matrix4 result;

    for (unsigned int i = 0; i < 16; ++i)
    {
        result.v[i] = v[i] - other.v[i];
    }

    return result;

}   // End of matrix4::operator-()


//-------------------------------------------------------------------------------
// @ matrix4::operator-=()
//-------------------------------------------------------------------------------
// Matrix subtraction by self
//-------------------------------------------------------------------------------
matrix4&
matrix4::operator-=(const matrix4& other)
{
    for (unsigned int i = 0; i < 16; ++i)
    {
        v[i] -= other.v[i];
    }

    return *this;

}   // End of matrix4::operator-=()


//-------------------------------------------------------------------------------
// @ matrix4::operator-=() (unary)
//-------------------------------------------------------------------------------
// Negate self and return
//-------------------------------------------------------------------------------
matrix4
matrix4::operator-() const
{
    matrix4 result;

    for (unsigned int i = 0; i < 16; ++i)
    {
        result.v[i] = -v[i];
    }

    return result;

}    // End of quaternion::operator-()


//-------------------------------------------------------------------------------
// @ matrix4::operator*()
//-------------------------------------------------------------------------------
// Matrix multiplication
//-------------------------------------------------------------------------------
matrix4  
matrix4::operator*(const matrix4& other) const
{
    matrix4 result;

    result.v[0] = v[0]*other.v[0] + v[4]*other.v[1] + v[8]*other.v[2] 
                    + v[12]*other.v[3];
    result.v[1] = v[1]*other.v[0] + v[5]*other.v[1] + v[9]*other.v[2] 
                    + v[13]*other.v[3];
    result.v[2] = v[2]*other.v[0] + v[6]*other.v[1] + v[10]*other.v[2] 
                    + v[14]*other.v[3];
    result.v[3] = v[3]*other.v[0] + v[7]*other.v[1] + v[11]*other.v[2] 
                    + v[15]*other.v[3];

    result.v[4] = v[0]*other.v[4] + v[4]*other.v[5] + v[8]*other.v[6] 
                    + v[12]*other.v[7];
    result.v[5] = v[1]*other.v[4] + v[5]*other.v[5] + v[9]*other.v[6] 
                    + v[13]*other.v[7];
    result.v[6] = v[2]*other.v[4] + v[6]*other.v[5] + v[10]*other.v[6] 
                    + v[14]*other.v[7];
    result.v[7] = v[3]*other.v[4] + v[7]*other.v[5] + v[11]*other.v[6] 
                    + v[15]*other.v[7];

    result.v[8] = v[0]*other.v[8] + v[4]*other.v[9] + v[8]*other.v[10] 
                    + v[12]*other.v[11];
    result.v[9] = v[1]*other.v[8] + v[5]*other.v[9] + v[9]*other.v[10] 
                    + v[13]*other.v[11];
    result.v[10] = v[2]*other.v[8] + v[6]*other.v[9] + v[10]*other.v[10] 
                    + v[14]*other.v[11];
    result.v[11] = v[3]*other.v[8] + v[7]*other.v[9] + v[11]*other.v[10] 
                    + v[15]*other.v[11];

    result.v[12] = v[0]*other.v[12] + v[4]*other.v[13] + v[8]*other.v[14] 
                    + v[12]*other.v[15];
    result.v[13] = v[1]*other.v[12] + v[5]*other.v[13] + v[9]*other.v[14] 
                    + v[13]*other.v[15];
    result.v[14] = v[2]*other.v[12] + v[6]*other.v[13] + v[10]*other.v[14] 
                    + v[14]*other.v[15];
    result.v[15] = v[3]*other.v[12] + v[7]*other.v[13] + v[11]*other.v[14] 
                    + v[15]*other.v[15];

    return result;

}   // End of matrix4::operator*()


//-------------------------------------------------------------------------------
// @ matrix4::operator*=()
//-------------------------------------------------------------------------------
// Matrix multiplication by self
//-------------------------------------------------------------------------------
matrix4&
matrix4::operator*=(const matrix4& other)
{
    matrix4 result;

    result.v[0] = v[0]*other.v[0] + v[4]*other.v[1] + v[8]*other.v[2] 
                    + v[12]*other.v[3];
    result.v[1] = v[1]*other.v[0] + v[5]*other.v[1] + v[9]*other.v[2] 
                    + v[13]*other.v[3];
    result.v[2] = v[2]*other.v[0] + v[6]*other.v[1] + v[10]*other.v[2] 
                    + v[14]*other.v[3];
    result.v[3] = v[3]*other.v[0] + v[7]*other.v[1] + v[11]*other.v[2] 
                    + v[15]*other.v[3];

    result.v[4] = v[0]*other.v[4] + v[4]*other.v[5] + v[8]*other.v[6] 
                    + v[12]*other.v[7];
    result.v[5] = v[1]*other.v[4] + v[5]*other.v[5] + v[9]*other.v[6] 
                    + v[13]*other.v[7];
    result.v[6] = v[2]*other.v[4] + v[6]*other.v[5] + v[10]*other.v[6] 
                    + v[14]*other.v[7];
    result.v[7] = v[3]*other.v[4] + v[7]*other.v[5] + v[11]*other.v[6] 
                    + v[15]*other.v[7];

    result.v[8] = v[0]*other.v[8] + v[4]*other.v[9] + v[8]*other.v[10] 
                    + v[12]*other.v[11];
    result.v[9] = v[1]*other.v[8] + v[5]*other.v[9] + v[9]*other.v[10] 
                    + v[13]*other.v[11];
    result.v[10] = v[2]*other.v[8] + v[6]*other.v[9] + v[10]*other.v[10] 
                    + v[14]*other.v[11];
    result.v[11] = v[3]*other.v[8] + v[7]*other.v[9] + v[11]*other.v[10] 
                    + v[15]*other.v[11];

    result.v[12] = v[0]*other.v[12] + v[4]*other.v[13] + v[8]*other.v[14] 
                    + v[12]*other.v[15];
    result.v[13] = v[1]*other.v[12] + v[5]*other.v[13] + v[9]*other.v[14] 
                    + v[13]*other.v[15];
    result.v[14] = v[2]*other.v[12] + v[6]*other.v[13] + v[10]*other.v[14] 
                    + v[14]*other.v[15];
    result.v[15] = v[3]*other.v[12] + v[7]*other.v[13] + v[11]*other.v[14] 
                    + v[15]*other.v[15];

    for (unsigned int i = 0; i < 16; ++i)
    {
        v[i] = result.v[i];
    }

    return *this;

}   // End of matrix4::operator*=()


//-------------------------------------------------------------------------------
// @ matrix4::operator*()
//-------------------------------------------------------------------------------
// Matrix-column vector multiplication
//-------------------------------------------------------------------------------
vector4   
matrix4::operator*(const vector4& other) const
{
    vector4 result;

    result.x = v[0]*other.x + v[4]*other.y + v[8]*other.z + v[12]*other.w;
    result.y = v[1]*other.x + v[5]*other.y + v[9]*other.z + v[13]*other.w;
    result.z = v[2]*other.x + v[6]*other.y + v[10]*other.z + v[14]*other.w;
    result.w = v[3]*other.x + v[7]*other.y + v[11]*other.z + v[15]*other.w;

    return result;

}   // End of matrix4::operator*()


//-------------------------------------------------------------------------------
// @ ::operator*()
//-------------------------------------------------------------------------------
// Matrix-row vector multiplication
//-------------------------------------------------------------------------------
vector4 
operator*(const vector4& vector, const matrix4& matrix)
{
    vector4 result;

    result.x = matrix.v[0]*vector.x + matrix.v[1]*vector.y 
             + matrix.v[2]*vector.z + matrix.v[3]*vector.w;
    result.y = matrix.v[4]*vector.x + matrix.v[5]*vector.y 
             + matrix.v[6]*vector.z + matrix.v[7]*vector.w;
    result.z = matrix.v[8]*vector.x + matrix.v[9]*vector.y 
             + matrix.v[10]*vector.z + matrix.v[11]*vector.w;
    result.w = matrix.v[12]*vector.x + matrix.v[13]*vector.y 
             + matrix.v[14]*vector.z + matrix.v[15]*vector.w;

    return result;

}   // End of matrix4::operator*()


//-------------------------------------------------------------------------------
// @ matrix4::*=()
//-------------------------------------------------------------------------------
// Matrix-scalar multiplication
//-------------------------------------------------------------------------------
matrix4& matrix4::operator*=(float scalar)
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
    v[9] *= scalar;
    v[10] *= scalar;
    v[11] *= scalar;
    v[12] *= scalar;
    v[13] *= scalar;
    v[14] *= scalar;
    v[15] *= scalar;

    return *this;
}  // End of matrix4::operator*=()


//-------------------------------------------------------------------------------
// @ friend matrix4 *()
//-------------------------------------------------------------------------------
// Matrix-scalar multiplication
//-------------------------------------------------------------------------------
matrix4 operator*(float scalar, const matrix4& matrix)
{
    matrix4 result;
    result.v[0] = matrix.v[0] * scalar;
    result.v[1] = matrix.v[1] * scalar;
    result.v[2] = matrix.v[2] * scalar;
    result.v[3] = matrix.v[3] * scalar;
    result.v[4] = matrix.v[4] * scalar;
    result.v[5] = matrix.v[5] * scalar;
    result.v[6] = matrix.v[6] * scalar;
    result.v[7] = matrix.v[7] * scalar;
    result.v[8] = matrix.v[8] * scalar;
    result.v[9] = matrix.v[9] * scalar;
    result.v[10] = matrix.v[10] * scalar;
    result.v[11] = matrix.v[11] * scalar;
    result.v[12] = matrix.v[12] * scalar;
    result.v[13] = matrix.v[13] * scalar;
    result.v[14] = matrix.v[14] * scalar;
    result.v[15] = matrix.v[15] * scalar;

    return result;
}  // End of friend matrix4 operator*()


//-------------------------------------------------------------------------------
// @ matrix4::*()
//-------------------------------------------------------------------------------
// Matrix-scalar multiplication
//-------------------------------------------------------------------------------
matrix4 matrix4::operator*(float scalar) const
{
    matrix4 result;
    result.v[0] = v[0] * scalar;
    result.v[1] = v[1] * scalar;
    result.v[2] = v[2] * scalar;
    result.v[3] = v[3] * scalar;
    result.v[4] = v[4] * scalar;
    result.v[5] = v[5] * scalar;
    result.v[6] = v[6] * scalar;
    result.v[7] = v[7] * scalar;
    result.v[8] = v[8] * scalar;
    result.v[9] = v[9] * scalar;
    result.v[10] = v[10] * scalar;
    result.v[11] = v[11] * scalar;
    result.v[12] = v[12] * scalar;
    result.v[13] = v[13] * scalar;
    result.v[14] = v[14] * scalar;
    result.v[15] = v[15] * scalar;

    return result;
}  // End of matrix4::operator*=()


//-------------------------------------------------------------------------------
// @ matrix4::transform()
//-------------------------------------------------------------------------------
// Matrix-point multiplication
//-------------------------------------------------------------------------------
vector3   
matrix4::transform(const vector3& other) const
{
    vector3 result;

    result.x = v[0]*other.x + v[4]*other.y + v[8]*other.z + v[12];
    result.y = v[1]*other.x + v[5]*other.y + v[9]*other.z + v[13];
    result.z = v[2]*other.x + v[6]*other.y + v[10]*other.z + v[14];
 
    return result;

}   // End of matrix4::transform()


//-------------------------------------------------------------------------------
// @ matrix4::transform_point()
//-------------------------------------------------------------------------------
// Matrix-point multiplication
//-------------------------------------------------------------------------------
point3   
matrix4::transform(const point3& other) const
{
    point3 result;

    result.x = v[0]*other.x + v[4]*other.y + v[8]*other.z + v[12];
    result.y = v[1]*other.x + v[5]*other.y + v[9]*other.z + v[13];
    result.z = v[2]*other.x + v[6]*other.y + v[10]*other.z + v[14];
 
    return result;

}   // End of matrix4::transform_point()

//-------------------------------------------------------------------------------
// @ matrix4::transform_point()
//-------------------------------------------------------------------------------
// Matrix-point multiplication
//-------------------------------------------------------------------------------
point3   
matrix4::rotate(const point3& other) const
{
    point3 result;

    result.x = v[0]*other.x + v[4]*other.y + v[8]*other.z;
    result.y = v[1]*other.x + v[5]*other.y + v[9]*other.z;
    result.z = v[2]*other.x + v[6]*other.y + v[10]*other.z;
 
    return result;

}   // End of matrix4::transform_point()
