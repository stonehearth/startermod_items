#include "pch.h"
#include "csg.h"
#include "quaternion.h"
#include "point.h"
#include "matrix.h"

using namespace ::radiant;
using namespace ::radiant::csg;

//-------------------------------------------------------------------------------
//-- Static Members -------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//-- Methods --------------------------------------------------------------------
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
// @ Matrix3::Matrix3()
//-------------------------------------------------------------------------------
// Axis-angle constructor
//-------------------------------------------------------------------------------
Matrix3::Matrix3(const Quaternion& quat)
{
    (void) rotation(quat);

}   // End of Matrix3::Matrix3()


//-------------------------------------------------------------------------------
// @ Matrix3::Matrix3()
//-------------------------------------------------------------------------------
// Copy constructor
//-------------------------------------------------------------------------------
Matrix3::Matrix3(const Matrix3& other)
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

}   // End of Matrix3::Matrix3()


//-------------------------------------------------------------------------------
// @ Matrix3::operator=()
//-------------------------------------------------------------------------------
// Assignment operator
//-------------------------------------------------------------------------------
Matrix3&
Matrix3::operator=(const Matrix3& other)
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

}   // End of Matrix3::operator=()


//-------------------------------------------------------------------------------
// @ operator<<()
//-------------------------------------------------------------------------------
// Text output for debugging
//-------------------------------------------------------------------------------
std::ostream& 
csg::operator<<(std::ostream& out, const Matrix3& source)
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
// @ Matrix3::operator==()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
Matrix3::operator==(const Matrix3& other) const
{
    for (unsigned int i = 0; i < 9; ++i)
    {
        if (!csg::AreEqual(v[i], other.v[i]))
            return false;
    }
    return true;

}   // End of Matrix3::operator==()


//-------------------------------------------------------------------------------
// @ Matrix3::operator!=()
//-------------------------------------------------------------------------------
// Comparison operator
//-------------------------------------------------------------------------------
bool 
Matrix3::operator!=(const Matrix3& other) const
{
    for (unsigned int i = 0; i < 9; ++i)
    {
        if (!csg::AreEqual(v[i], other.v[i]))
            return true;
    }
    return false;

}   // End of Matrix3::operator!=()


//-------------------------------------------------------------------------------
// @ Matrix3::IsZero()
//-------------------------------------------------------------------------------
// Check for zero matrix
//-------------------------------------------------------------------------------
bool 
Matrix3::IsZero() const
{
    for (unsigned int i = 0; i < 9; ++i)
    {
        if (!csg::IsZero(v[i]))
            return false;
    }
    return true;

}   // End of Matrix3::IsZero()


//-------------------------------------------------------------------------------
// @ Matrix3::is_identity()
//-------------------------------------------------------------------------------
// Check for identity matrix
//-------------------------------------------------------------------------------
bool 
Matrix3::is_identity() const
{
    return ::csg::AreEqual(1.0f, v[0])
        && ::csg::AreEqual(1.0f, v[4])
        && ::csg::AreEqual(1.0f, v[8])
        && csg::IsZero(v[1]) 
        && csg::IsZero(v[2])
        && csg::IsZero(v[3])
        && csg::IsZero(v[5]) 
        && csg::IsZero(v[6])
        && csg::IsZero(v[7]);

}   // End of Matrix3::is_identity()


//-------------------------------------------------------------------------------
// @ Matrix3::set_rows()
//-------------------------------------------------------------------------------
// Set matrix, row by row
//-------------------------------------------------------------------------------
void 
Matrix3::set_rows(const Point3f& row1, const Point3f& row2, const Point3f& row3)
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

}   // End of Matrix3::set_rows()


//-------------------------------------------------------------------------------
// @ Matrix3::get_rows()
//-------------------------------------------------------------------------------
// Get matrix, row by row
//-------------------------------------------------------------------------------
void 
Matrix3::get_rows(Point3f& row1, Point3f& row2, Point3f& row3) const
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
}   // End of Matrix3::get_rows()


//-------------------------------------------------------------------------------
// @ Matrix3::GetRow()
//-------------------------------------------------------------------------------
// Get matrix, row by row
//-------------------------------------------------------------------------------
Point3f 
Matrix3::GetRow(unsigned int i) const
{
    ASSERT(i < 3);
    return Point3f(v[i], v[i+3], v[i+6]);

}   // End of Matrix3::GetRow()


//-------------------------------------------------------------------------------
// @ Matrix3::set_columns()
//-------------------------------------------------------------------------------
// Set matrix, row by row
//-------------------------------------------------------------------------------
void 
Matrix3::set_columns(const Point3f& col1, const Point3f& col2, const Point3f& col3)
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

}   // End of Matrix3::set_columns()


//-------------------------------------------------------------------------------
// @ Matrix3::get_columns()
//-------------------------------------------------------------------------------
// Get matrix, row by row
//-------------------------------------------------------------------------------
void 
Matrix3::get_columns(Point3f& col1, Point3f& col2, Point3f& col3) const
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

}   // End of Matrix3::get_columns()


//-------------------------------------------------------------------------------
// @ Matrix3::get_columns()
//-------------------------------------------------------------------------------
// Get matrix, row by row
//-------------------------------------------------------------------------------
Point3f 
Matrix3::get_column(unsigned int i) const 
{
    ASSERT(i < 3);
    return Point3f(v[3*i], v[3*i+1], v[3*i+2]);

}   // End of Matrix3::get_columns()


//-------------------------------------------------------------------------------
// @ Matrix3::clean()
//-------------------------------------------------------------------------------
// Set elements close to zero equal to zero
//-------------------------------------------------------------------------------
void
Matrix3::clean()
{
    for (unsigned int i = 0; i < 9; ++i)
    {
        if (csg::IsZero(v[i]))
            v[i] = 0.0f;
    }

}   // End of Matrix3::clean()


//-------------------------------------------------------------------------------
// @ Matrix3::identity()
//-------------------------------------------------------------------------------
// Set to identity matrix
//-------------------------------------------------------------------------------
void
Matrix3::SetIdentity()
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

}   // End of Matrix3::identity()


//-------------------------------------------------------------------------------
// @ Matrix3::inverse()
//-------------------------------------------------------------------------------
// Set self to inverse
//-------------------------------------------------------------------------------
Matrix3& 
Matrix3::inverse()
{
    *this = csg::inverse(*this);

    return *this;

}   // End of Matrix3::inverse()


//-------------------------------------------------------------------------------
// @ Matrix3::inverse()
//-------------------------------------------------------------------------------
// Compute matrix inverse
//-------------------------------------------------------------------------------
Matrix3 
csg::inverse(const Matrix3& mat)
{
    Matrix3 result;
    
    // compute determinant
    float cofactor0 = mat.v[4]*mat.v[8] - mat.v[5]*mat.v[7];
    float cofactor3 = mat.v[2]*mat.v[7] - mat.v[1]*mat.v[8];
    float cofactor6 = mat.v[1]*mat.v[5] - mat.v[2]*mat.v[4];
    float det = mat.v[0]*cofactor0 + mat.v[3]*cofactor3 + mat.v[6]*cofactor6;
    if (csg::IsZero(det))
    {
        ASSERT(false);
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

}   // End of Matrix3::inverse()


//-------------------------------------------------------------------------------
// @ Matrix3::transpose()
//-------------------------------------------------------------------------------
// Set self to transpose
//-------------------------------------------------------------------------------
Matrix3& 
Matrix3::transpose()
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

}   // End of Matrix3::transpose()


//-------------------------------------------------------------------------------
// @ Matrix3::transpose()
//-------------------------------------------------------------------------------
// Compute matrix transpose
//-------------------------------------------------------------------------------
Matrix3 
csg::transpose(const Matrix3& mat)
{
    Matrix3 result;

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

}   // End of Matrix3::transpose()


//-------------------------------------------------------------------------------
// @ Matrix3::determinant()
//-------------------------------------------------------------------------------
// Get determinant of matrix
//-------------------------------------------------------------------------------
float 
Matrix3::determinant() const
{
    return v[0]*(v[4]*v[8] - v[5]*v[7]) 
         + v[3]*(v[2]*v[7] - v[1]*v[8])
         + v[6]*(v[1]*v[5] - v[2]*v[4]);

}   // End of Matrix3::determinant()


//-------------------------------------------------------------------------------
// @ Matrix3::Adjoint()
//-------------------------------------------------------------------------------
// Compute matrix adjoint
//-------------------------------------------------------------------------------
Matrix3 
Matrix3::Adjoint() const
{
    Matrix3 result;
    
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

}   // End of Matrix3::Adjoint()


//-------------------------------------------------------------------------------
// @ Matrix3::Guard()
//-------------------------------------------------------------------------------
// Get trace of matrix
//-------------------------------------------------------------------------------
float 
Matrix3::Guard() const
{
    return v[0] + v[4] + v[8];

}   // End of Matrix3::Guard()


//-------------------------------------------------------------------------------
// @ Matrix3::rotation()
//-------------------------------------------------------------------------------
// Set as rotation matrix based on Quaternion
//-------------------------------------------------------------------------------
Matrix3& 
Matrix3::rotation(const Quaternion& rotate)
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
// @ Matrix3::rotation()
// ---------------------------------------------------------------------------
// Sets the matrix to a rotation matrix (by Euler angles).
//----------------------------------------------------------------------------
Matrix3 &
Matrix3::rotation(float z_rotation, float y_rotation, float x_rotation)
{
    // This is an "unrolled" contatenation of rotation matrices X Y & Z
    float Cx, Sx;
    csg::SinCos(x_rotation, Sx, Cx);

    float Cy, Sy;
    csg::SinCos(y_rotation, Sy, Cy);

    float Cz, Sz;
    csg::SinCos(z_rotation, Sz, Cz);

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

}  // End of Matrix3::rotation()


//----------------------------------------------------------------------------
// @ Matrix3::rotation()
// ---------------------------------------------------------------------------
// Sets the matrix to a rotation matrix (by axis and angle).
//----------------------------------------------------------------------------
Matrix3 &
Matrix3::rotation(const Point3f& axis, float angle)
{
    float c, s;
    csg::SinCos(angle, s, c);
    float t = 1.0f - c;

    Point3f nAxis = axis;
    nAxis.Normalize();

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

}  // End of Matrix3::rotation()


//-------------------------------------------------------------------------------
// @ Matrix3::scaling()
//-------------------------------------------------------------------------------
// Set as scaling matrix based on vector
//-------------------------------------------------------------------------------
Matrix3& 
Matrix3::scaling(const Point3f& scaleFactors)
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
// @ Matrix3::rotation_x()
//-------------------------------------------------------------------------------
// Set as rotation matrix, rotating by 'angle' radians around x-axis
//-------------------------------------------------------------------------------
Matrix3& 
Matrix3::rotation_x(float angle)
{
    float sintheta, costheta;
    csg::SinCos(angle, sintheta, costheta);

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
// @ Matrix3::rotation_y()
//-------------------------------------------------------------------------------
// Set as rotation matrix, rotating by 'angle' radians around y-axis
//-------------------------------------------------------------------------------
Matrix3& 
Matrix3::rotation_y(float angle)
{
    float sintheta, costheta;
    csg::SinCos(angle, sintheta, costheta);

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
// @ Matrix3::rotation_z()
//-------------------------------------------------------------------------------
// Set as rotation matrix, rotating by 'angle' radians around z-axis
//-------------------------------------------------------------------------------
Matrix3& 
Matrix3::rotation_z(float angle)
{
    float sintheta, costheta;
    csg::SinCos(angle, sintheta, costheta);

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
// @ Matrix3::get_fixed_angles()
// ---------------------------------------------------------------------------
// Gets one set of possible z-y-x fixed angles that will generate this matrix
// Assumes that this is a rotation matrix
//----------------------------------------------------------------------------
void
Matrix3::get_fixed_angles(float& z_rotation, float& y_rotation, float& x_rotation)
{
    float Cx, Sx;
    float Cy, Sy;
    float Cz, Sz;

    Sy = v[6];
    Cy = csg::Sqrt(1.0f - Sy*Sy);
    // normal case
    if (!csg::IsZero(Cy))
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

}  // End of Matrix3::get_fixed_angles()


//----------------------------------------------------------------------------
// @ Matrix3::get_axis_angle()
// ---------------------------------------------------------------------------
// Gets one possible axis-angle pair that will generate this matrix
// Assumes that this is a rotation matrix
//----------------------------------------------------------------------------
void
Matrix3::get_axis_angle(Point3f& axis, float& angle)
{
    float trace = v[0] + v[4] + v[8];
    float cosTheta = 0.5f*(trace - 1.0f);
    angle = acosf(cosTheta);

    // angle is zero, axis can be anything
    if (csg::IsZero(angle))
    {
        axis = Point3f::unitX;
    }
    // standard case
    else if (angle < k_pi-k_epsilon)
    {
        axis = Point3f(v[5]-v[7], v[6]-v[2], v[1]-v[3]);
        axis.Normalize();
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
        float s = csg::Sqrt(v[i + 3*i] - v[j + 3*j] - v[k + 3*k] + 1.0f);
        axis[i] = 0.5f*s;
        float recip = 1.0f/s;
        axis[j] = (v[i + 3*j])*recip;
        axis[k] = (v[k + 3*i])*recip;
    }

}  // End of Matrix3::get_axis_angle()


//-------------------------------------------------------------------------------
// @ Matrix3::operator+()
//-------------------------------------------------------------------------------
// Matrix addition 
//-------------------------------------------------------------------------------
Matrix3
Matrix3::operator+(const Matrix3& other) const
{
    Matrix3 result;

    for (unsigned int i = 0; i < 9; ++i)
    {
        result.v[i] = v[i] + other.v[i];
    }

    return result;

}   // End of Matrix3::operator-()


//-------------------------------------------------------------------------------
// @ Matrix3::operator+=()
//-------------------------------------------------------------------------------
// Matrix addition by self
//-------------------------------------------------------------------------------
Matrix3&
Matrix3::operator+=(const Matrix3& other)
{
    for (unsigned int i = 0; i < 9; ++i)
    {
        v[i] += other.v[i];
    }

    return *this;

}   // End of Matrix3::operator+=()


//-------------------------------------------------------------------------------
// @ Matrix3::operator-()
//-------------------------------------------------------------------------------
// Matrix subtraction 
//-------------------------------------------------------------------------------
Matrix3
Matrix3::operator-(const Matrix3& other) const
{
    Matrix3 result;

    for (unsigned int i = 0; i < 9; ++i)
    {
        result.v[i] = v[i] - other.v[i];
    }

    return result;

}   // End of Matrix3::operator-()


//-------------------------------------------------------------------------------
// @ Matrix3::operator-=()
//-------------------------------------------------------------------------------
// Matrix subtraction by self
//-------------------------------------------------------------------------------
Matrix3&
Matrix3::operator-=(const Matrix3& other)
{
    for (unsigned int i = 0; i < 9; ++i)
    {
        v[i] -= other.v[i];
    }

    return *this;

}   // End of Matrix3::operator-=()


//-------------------------------------------------------------------------------
// @ Matrix3::operator-=() (unary)
//-------------------------------------------------------------------------------
// Negate self and return
//-------------------------------------------------------------------------------
Matrix3
Matrix3::operator-() const
{
    Matrix3 result;

    for (unsigned int i = 0; i < 16; ++i)
    {
        result.v[i] = -v[i];
    }

    return result;

}    // End of Quaternion::operator-()


//-------------------------------------------------------------------------------
// @ Matrix3::operator*()
//-------------------------------------------------------------------------------
// Matrix multiplication
//-------------------------------------------------------------------------------
Matrix3  
Matrix3::operator*(const Matrix3& other) const
{
    Matrix3 result;

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

}   // End of Matrix3::operator*()


//-------------------------------------------------------------------------------
// @ Matrix3::operator*=()
//-------------------------------------------------------------------------------
// Matrix multiplication by self
//-------------------------------------------------------------------------------
Matrix3&
Matrix3::operator*=(const Matrix3& other)
{
    Matrix3 result;

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

}   // End of Matrix3::operator*=()


//-------------------------------------------------------------------------------
// @ Matrix3::operator*()
//-------------------------------------------------------------------------------
// Matrix-column vector multiplication
//-------------------------------------------------------------------------------
Point3f   
Matrix3::operator*(const Point3f& other) const
{
    Point3f result;

    result.x = v[0]*other.x + v[3]*other.y + v[6]*other.z;
    result.y = v[1]*other.x + v[4]*other.y + v[7]*other.z;
    result.z = v[2]*other.x + v[5]*other.y + v[8]*other.z;

    return result;

}   // End of Matrix3::operator*()

//-------------------------------------------------------------------------------
// @ Matrix3::operator*()
//-------------------------------------------------------------------------------
// Row vector-matrix multiplication
//-------------------------------------------------------------------------------
Point3f   
csg::operator*(const Point3f& vector, const Matrix3& mat)
{
    Point3f result;

    result.x = mat.v[0]*vector.x + mat.v[1]*vector.y + mat.v[2]*vector.z;
    result.y = mat.v[3]*vector.x + mat.v[4]*vector.y + mat.v[5]*vector.z;
    result.z = mat.v[6]*vector.x + mat.v[7]*vector.y + mat.v[8]*vector.z;

    return result;

}   // End of Matrix3::operator*()

//-------------------------------------------------------------------------------
// @ Matrix3::*=()
//-------------------------------------------------------------------------------
// Matrix-scalar multiplication
//-------------------------------------------------------------------------------
Matrix3& Matrix3::operator*=(float scalar)
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
}  // End of Matrix3::operator*=()


//-------------------------------------------------------------------------------
// @ friend Matrix3 *()
//-------------------------------------------------------------------------------
// Matrix-scalar multiplication
//-------------------------------------------------------------------------------
Matrix3 operator*(float scalar, const Matrix3& matrix)
{
    Matrix3 result;
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
}  // End of friend Matrix3 operator*()


//-------------------------------------------------------------------------------
// @ Matrix3::*()
//-------------------------------------------------------------------------------
// Matrix-scalar multiplication
//-------------------------------------------------------------------------------
Matrix3 Matrix3::operator*(float scalar) const
{
    Matrix3 result;
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
}  // End of Matrix3::operator*=()


