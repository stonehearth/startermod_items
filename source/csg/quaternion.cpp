#include "pch.h"
#include "quaternion.h"
#include "point.h"
#include "matrix.h"

using namespace ::radiant;
using namespace ::radiant::csg;

Quaternion Quaternion::zero(0.0f, 0.0f, 0.0f, 0.0f);
Quaternion Quaternion::identity(1.0f, 0.0f, 0.0f, 0.0f);


Quaternion::Quaternion(const Point3f& axis, float angle)
{
    set(axis, angle);
}   // End of Quaternion::Quaternion()


Quaternion::Quaternion(const Point3f& from, const Point3f& to)
{
    set(from, to);
}   // End of Quaternion::Quaternion()

Quaternion::Quaternion(const Point3f& vector)
{
    set(0.0f, vector.x, vector.y, vector.z);
}   // End of Quaternion::Quaternion()

Quaternion::Quaternion(const Matrix3& rotation)
{
    float trace = rotation.Guard();
    if (trace > 0.0f)
    {
        float s = sqrt(trace + 1.0f);
        w = s*0.5f;
        float recip = 0.5f/s;
        x = (rotation(2,1) - rotation(1,2))*recip;
        y = (rotation(0,2) - rotation(2,0))*recip;
        z = (rotation(1,0) - rotation(0,1))*recip;
    }
    else
    {
        unsigned int i = 0;
        if (rotation(1,1) > rotation(0,0))
            i = 1;
        if (rotation(2,2) > rotation(i,i))
            i = 2;
        unsigned int j = (i+1)%3;
        unsigned int k = (j+1)%3;

        float s = sqrt(rotation(i,i) - rotation(j,j) - rotation(k,k) + 1.0f);

        float* apkQuat[3] = { &x, &y, &z };
        *apkQuat[i] = 0.5f*s;
        float recip = 0.5f/s;
        w = (rotation(k,j) - rotation(j,k))*recip;
        *apkQuat[j] = (rotation(j,i) + rotation(i,j))*recip;
        *apkQuat[k] = (rotation(k,i) + rotation(i,k))*recip;
    }

} 

Quaternion::Quaternion(const Quaternion& other) : 
    w(other.w),
    x(other.x),
    y(other.y),
    z(other.z)
{

}

Quaternion&
Quaternion::operator=(const Quaternion& other)
{
    // if same object
    if (this == &other)
        return *this;
        
    w = other.w;
    x = other.x;
    y = other.y;
    z = other.z;
    
    return *this;

} 

std::ostream& 
csg::operator<<(std::ostream& out, const Quaternion& source)
{
    out << '[' << source.w << ',' << source.x << ',' 
        << source.y << ',' << source.z << ']';

    return out;
    
}   // End of operator<<()
    

float 
Quaternion::magnitude() const
{
    return sqrt(w*w + x*x + y*y + z*z);

}

float 
Quaternion::norm() const
{
    return (w*w + x*x + y*y + z*z);

}

bool 
Quaternion::operator==(const Quaternion& other) const
{
    if (csg::IsZero(other.w - w)
        && csg::IsZero(other.x - x)
        && csg::IsZero(other.y - y)
        && csg::IsZero(other.z - z))
        return true;

    return false;   
}

bool 
Quaternion::operator!=(const Quaternion& other) const
{
    if (csg::IsZero(other.w - w)
        || csg::IsZero(other.x - x)
        || csg::IsZero(other.y - y)
        || csg::IsZero(other.z - z))
        return false;

    return true;
}

bool 
Quaternion::IsZero() const
{
    return csg::IsZero(w*w + x*x + y*y + z*z);

} 

bool 
Quaternion::is_unit() const
{
    return csg::IsZero(1.0f - magnitude());
} 

bool 
Quaternion::is_identity() const
{
    return csg::IsZero(1.0f - w)
        && csg::IsZero(x) 
        && csg::IsZero(y)
        && csg::IsZero(z);

}

void
Quaternion::set(const Point3f& axis, float angle)
{
    // if axis of rotation is zero vector, just set to identity quat
    float length = axis.LengthSquared();
    if (csg::IsZero(length))
    {
        SetIdentity();
        return;
    }

    // take half-angle
    angle *= 0.5f;

    float sintheta, costheta;
    csg::SinCos(angle, sintheta, costheta);

    float scaleFactor = sintheta/sqrt(length);

    w = costheta;
    x = scaleFactor * axis.x;
    y = scaleFactor * axis.y;
    z = scaleFactor * axis.z;

}

void
Quaternion::set(const Point3f& from, const Point3f& to)
{
   // get axis of rotation
    Point3f axis = from.Cross(to);

    // get scaled cos of angle between vectors and set initial Quaternion
    set( from.Dot(to), axis.x, axis.y, axis.z);
    // Quaternion at this point is ||from||*||to||*(cos(theta), r*Sin(theta))

    // Normalize to remove ||from||*||to|| factor
    Normalize();
    // Quaternion at this point is (cos(theta), r*Sin(theta))
    // what we want is (cos(theta/2), r*Sin(theta/2))

    // set up for half angle calculation
    w += 1.0f;

    // now when we Normalize, we'll be dividing by sqrt(2*(1+cos(theta))), which is 
    // what we want for r*Sin(theta) to give us r*Sin(theta/2)  (see pages 487-488)
    // 
    // w will become 
    //                 1+cos(theta)
    //            ----------------------
    //            sqrt(2*(1+cos(theta)))        
    // which simplifies to
    //                cos(theta/2)

    // before we Normalize, check if vectors are opposing
    if (w <= k_epsilon)
    {
        // rotate pi radians around orthogonal vector
        // take Cross product with x axis
        if (from.z*from.z > from.x*from.x)
            set(0.0f, 0.0f, from.z, -from.y);
        // or take Cross product with z axis
        else
            set(0.0f, from.y, -from.x, 0.0f);
    }
   
    // Normalize again to get rotation Quaternion
    Normalize();

}

void 
Quaternion::set(float z_rotation, float y_rotation, float x_rotation) 
{
    z_rotation *= 0.5f;
    y_rotation *= 0.5f;
    x_rotation *= 0.5f;

    // get sines and cosines of half angles
    float Cx, Sx;
    csg::SinCos(x_rotation, Sx, Cx);

    float Cy, Sy;
    csg::SinCos(y_rotation, Sy, Cy);

    float Cz, Sz;
    csg::SinCos(z_rotation, Sz, Cz);

    // multiply it out
    w = Cx*Cy*Cz - Sx*Sy*Sz;
    x = Sx*Cy*Cz + Cx*Sy*Sz;
    y = Cx*Sy*Cz - Sx*Cy*Sz;
    z = Cx*Cy*Sz + Sx*Sy*Cx;

}

void
Quaternion::get_axis_angle(Point3f& axis, float& angle)
{
    angle = 2.0f*acosf(w);
    float length = sqrt(1.0f - w*w);
    if (csg::IsZero(length))
        axis.SetZero();
    else
    {
        length = 1.0f/length;
        axis = Point3f(x*length, y*length, z*length);
    }

}

void
Quaternion::clean()
{
    if (csg::IsZero(w))
        w = 0.0f;
    if (csg::IsZero(x))
        x = 0.0f;
    if (csg::IsZero(y))
        y = 0.0f;
    if (csg::IsZero(z))
        z = 0.0f;

}

void
Quaternion::Normalize()
{
    float lengthsq = w*w + x*x + y*y + z*z;

    if (csg::IsZero(lengthsq))
    {
        SetZero();
    }
    else
    {
        float factor = csg::InvSqrt(lengthsq);
        w *= factor;
        x *= factor;
        y *= factor;
        z *= factor;
    }

}

Quaternion 
csg::conjugate(const Quaternion& quat) 
{
    return Quaternion(quat.w, -quat.x, -quat.y, -quat.z);

}

const Quaternion& 
Quaternion::conjugate()
{
    x = -x;
    y = -y;
    z = -z;

    return *this;

}

Quaternion 
csg::inverse(const Quaternion& quat)
{
    float magnitude = quat.w*quat.w + quat.x*quat.x + quat.y*quat.y + quat.z*quat.z;
    // if we're the zero Quaternion, just return identity
    if (!csg::IsZero(magnitude))
    {
        ASSERT(false);
        return Quaternion();
    }

    float normRecip = 1.0f / magnitude;
    return Quaternion(normRecip*quat.w, -normRecip*quat.x, -normRecip*quat.y, 
                   -normRecip*quat.z);

}

const Quaternion& 
Quaternion::inverse()
{
    float magnitude = w*w + x*x + y*y + z*z;
    // if we're the zero Quaternion, just return
    if (csg::IsZero(magnitude))
        return *this;

    float normRecip = 1.0f / magnitude;
    w = normRecip*w;
    x = -normRecip*x;
    y = -normRecip*y;
    z = -normRecip*z;

    return *this;

}

Quaternion 
Quaternion::operator+(const Quaternion& other) const
{
    return Quaternion(w + other.w, x + other.x, y + other.y, z + other.z);

}   // End of Quaternion::operator+()


//-------------------------------------------------------------------------------
// @ Quaternion::operator+=()
//-------------------------------------------------------------------------------
// Add quat to self, store in self
//-------------------------------------------------------------------------------
Quaternion& 
Quaternion::operator+=(const Quaternion& other)
{
    w += other.w;
    x += other.x;
    y += other.y;
    z += other.z;

    return *this;

}   // End of Quaternion::operator+=()


//-------------------------------------------------------------------------------
// @ Quaternion::operator-()
//-------------------------------------------------------------------------------
// Subtract quat from self and return
//-------------------------------------------------------------------------------
Quaternion 
Quaternion::operator-(const Quaternion& other) const
{
    return Quaternion(w - other.w, x - other.x, y - other.y, z - other.z);

}   // End of Quaternion::operator-()


//-------------------------------------------------------------------------------
// @ Quaternion::operator-=()
//-------------------------------------------------------------------------------
// Subtract quat from self, store in self
//-------------------------------------------------------------------------------
Quaternion& 
Quaternion::operator-=(const Quaternion& other)
{
    w -= other.w;
    x -= other.x;
    y -= other.y;
    z -= other.z;

    return *this;

}   // End of Quaternion::operator-=()


//-------------------------------------------------------------------------------
// @ Quaternion::operator-=() (unary)
//-------------------------------------------------------------------------------
// Negate self and return
//-------------------------------------------------------------------------------
Quaternion
Quaternion::operator-() const
{
    return Quaternion(-w, -x, -y, -z);
}    // End of Quaternion::operator-()


//-------------------------------------------------------------------------------
// @ Quaternion::operator*=()
//-------------------------------------------------------------------------------
// Scalar multiplication by self
//-------------------------------------------------------------------------------
Quaternion&
Quaternion::operator*=(float scalar)
{
    w *= scalar;
    x *= scalar;
    y *= scalar;
    z *= scalar;

    return *this;

}   // End of Quaternion::operator*=()


//-------------------------------------------------------------------------------
// @ Quaternion::operator*()
//-------------------------------------------------------------------------------
// Quaternion multiplication
//-------------------------------------------------------------------------------
Quaternion  
Quaternion::operator*(const Quaternion& other) const
{
    return Quaternion(w*other.w - x*other.x - y*other.y - z*other.z,
                   w*other.x + x*other.w + y*other.z - z*other.y,
                   w*other.y + y*other.w + z*other.x - x*other.z,
                   w*other.z + z*other.w + x*other.y - y*other.x);

}   // End of Quaternion::operator*()


//-------------------------------------------------------------------------------
// @ Quaternion::operator*=()
//-------------------------------------------------------------------------------
// Quaternion multiplication by self
//-------------------------------------------------------------------------------
Quaternion&
Quaternion::operator*=(const Quaternion& other)
{
    set(w*other.w - x*other.x - y*other.y - z*other.z,
         w*other.x + x*other.w + y*other.z - z*other.y,
         w*other.y + y*other.w + z*other.x - x*other.z,
         w*other.z + z*other.w + x*other.y - y*other.x);
  
    return *this;

}   // End of Quaternion::operator*=()


//-------------------------------------------------------------------------------
// @ Quaternion::Dot()
//-------------------------------------------------------------------------------
// Dot product by self
//-------------------------------------------------------------------------------
float               
Quaternion::Dot(const Quaternion& quat) const
{
    return (w*quat.w + x*quat.x + y*quat.y + z*quat.z);

}   // End of Quaternion::Dot()


//-------------------------------------------------------------------------------
// @ Quaternion::rotate()
//-------------------------------------------------------------------------------
// rotate vector by Quaternion
// Assumes Quaternion is normalized!
//-------------------------------------------------------------------------------
Point3f   
Quaternion::rotate(const Point3f& vector) const
{
    ASSERT(is_unit());

    float v_mult = 2.0f*(x*vector.x + y*vector.y + z*vector.z);
    float cross_mult = 2.0f*w;
    float p_mult = cross_mult*w - 1.0f;

    return Point3f(p_mult*vector.x + v_mult*x + cross_mult*(y*vector.z - z*vector.y),
                   p_mult*vector.y + v_mult*y + cross_mult*(z*vector.x - x*vector.z),
                   p_mult*vector.z + v_mult*z + cross_mult*(x*vector.y - y*vector.x));

}   // End of Quaternion::rotate()

//-------------------------------------------------------------------------------
// @ lerp()
//-------------------------------------------------------------------------------
// Linearly interpolate two Quaternions
// This will always take the shorter path between them
//-------------------------------------------------------------------------------
void 
csg::lerp(Quaternion& result, const Quaternion& start, const Quaternion& end, float t)
{
    // get cos of "angle" between Quaternions
    float cosTheta = start.Dot(end);

    // initialize result
    result = t*end;

    // if "angle" between Quaternions is less than 90 degrees
    if (cosTheta >= k_epsilon)
    {
        // use standard interpolation
        result += (1.0f-t)*start;
    }
    else
    {
        // otherwise, take the shorter path
        result += (t-1.0f)*start;
    }

}   // End of lerp()


//-------------------------------------------------------------------------------
// @ slerp()
//-------------------------------------------------------------------------------
// Spherical linearly interpolate two Quaternions
// This will always take the shorter path between them
//-------------------------------------------------------------------------------
void 
csg::slerp(Quaternion& result, const Quaternion& start, const Quaternion& end, float t)
{
   ASSERT(t >= 0.0f && t <= 1.0f);

   // get cosine of "angle" between Quaternions
   float cosTheta = start.Dot(end);
   float startInterp, endInterp;

   // if "angle" between Quaternions is less than 90 degrees
   if (cosTheta >= k_epsilon)
   {
      // if angle is greater than zero
      if ((1.0f - cosTheta) > k_epsilon)
      {
         // use standard slerp
         float theta = acosf(cosTheta);
         float recipSinTheta = 1.0f/csg::Sin(theta);

         startInterp = csg::Sin((1.0f - t)*theta)*recipSinTheta;
         endInterp = csg::Sin(t*theta)*recipSinTheta;
      }
      // angle is close to zero
      else
      {
         // use linear interpolation
         startInterp = 1.0f - t;
         endInterp = t;
      }
   }
   // otherwise, take the shorter route
   else
   {
      // if angle is less than 180 degrees
      if ((1.0f + cosTheta) > k_epsilon)
      {
         // use slerp w/negation of start Quaternion
         float theta = acosf(-cosTheta);
         float recipSinTheta = 1.0f/csg::Sin(theta);

         startInterp = csg::Sin((t-1.0f)*theta)*recipSinTheta;
         endInterp = csg::Sin(t*theta)*recipSinTheta;
      }
      // angle is close to 180 degrees
      else
      {
         // use lerp w/negation of start Quaternion
         startInterp = t - 1.0f;
         endInterp = t;
      }
   }

   result = startInterp*start + endInterp*end;

}   // End of slerp()


//-------------------------------------------------------------------------------
// @ approx_slerp()
//-------------------------------------------------------------------------------
// Approximate spherical linear interpolation of two Quaternions
// Based on "Hacking Quaternions", Jonathan Blow, Game Developer, March 2002.
// See Game Developer, February 2004 for an alternate method.
//-------------------------------------------------------------------------------
void 
csg::approx_slerp(Quaternion& result, const Quaternion& start, const Quaternion& end, float t)
{
    float cosTheta = start.Dot(end);

    // correct time by using cosine of angle between Quaternions
    float factor = 1.0f - 0.7878088f*cosTheta;
    float k = 0.5069269f;
    factor *= factor;
    k *= factor;

    float b = 2*k;
    float c = -3*k;
    float d = 1 + k;

    t = t*(b*t + c) + d;

    // initialize result
    result = t*end;

    // if "angle" between Quaternions is less than 90 degrees
    if (cosTheta >= k_epsilon)
    {
        // use standard interpolation
        result += (1.0f-t)*start;
    }
    else
    {
        // otherwise, take the shorter path
        result += (t-1.0f)*start;
    }

}   // End of approx_slerp()

