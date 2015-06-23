#include "pch.h"
#include "quaternion.h"
#include "point.h"
#include "matrix.h"
#include "protocols/store.pb.h"

using namespace ::radiant;
using namespace ::radiant::csg;

Quaternion Quaternion::zero(0.0f, 0.0f, 0.0f, 0.0f);
Quaternion Quaternion::identity(1.0f, 0.0f, 0.0f, 0.0f);


Quaternion::Quaternion(const Point3f& axis, double angle)
{
    set(axis, angle);
}   // End of Quaternion::Quaternion()


Quaternion::Quaternion(const Point3f& from, const Point3f& to)
{
    set(from, to);
}   // End of Quaternion::Quaternion()

Quaternion::Quaternion(const Point3f& eulerAngle)
{
    double pitch = eulerAngle.x; // bank
    double yaw = eulerAngle.y; // heading11
    double roll = eulerAngle.z; //attitude

    double c1 = cos(yaw/2);
    double c2 = cos(roll/2);
    double c3 = cos(pitch/2);

    double s1 = sin(yaw/2);
    double s2 = sin(roll/2);
    double s3 = sin(pitch/2);

    double w = c1 * c2 * c3 - s1 * s2 * s3;
    double x = s1 * s2 * c3 + c1 * c2 * s3;
    double y = s1 * c2 * c3 + c1 * s2 * s3;
    double z = c1 * s2 * c3 - s1 * c2 * s3;

    set(w, x, y, z);
}   // End of Quaternion::Quaternion()

Quaternion::Quaternion(const Matrix3& rotation)
{
    double trace = rotation.Guard();
    if (trace > 0.0f)
    {
        double s = sqrt(trace + 1.0f);
        w = s*0.5f;
        double recip = 0.5f/s;
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

        double s = sqrt(rotation(i,i) - rotation(j,j) - rotation(k,k) + 1.0f);

        double* apkQuat[3] = { &x, &y, &z };
        *apkQuat[i] = 0.5f*s;
        double recip = 0.5f/s;
        w = (rotation(k,j) - rotation(j,k))*recip;
        *apkQuat[j] = (rotation(j,i) + rotation(i,j))*recip;
        *apkQuat[k] = (rotation(k,i) + rotation(i,k))*recip;
    }
}

Quaternion::Quaternion(const Matrix4& rotation)
{
    double trace = rotation(0,0) + rotation(1,1) + rotation(2,2);
    if (trace > 0.0f)
    {
        double s = sqrt(trace + 1.0f);
        w = s*0.5f;
        double recip = 0.5f/s;
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

        double s = sqrt(rotation(i,i) - rotation(j,j) - rotation(k,k) + 1.0f);

        double* apkQuat[3] = { &x, &y, &z };
        *apkQuat[i] = 0.5f*s;
        double recip = 0.5f/s;
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
    

double 
Quaternion::magnitude() const
{
    return sqrt(w*w + x*x + y*y + z*z);

}

double 
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
Quaternion::set(const Point3f& axis, double angle)
{
   // if axis of rotation is zero vector, just set to identity quat
   double lengthSquared = axis.LengthSquared();
   if (csg::IsZero(lengthSquared)) {
      SetIdentity();
      return;
   }

   // recall the rotation angle = 2*theta
   double theta = angle * 0.5f;

   double sinTheta, cosTheta;
   csg::SinCos(theta, sinTheta, cosTheta);

   // divide by the length in case the axis is not a unit vector
   double k = sinTheta / std::sqrt(lengthSquared);

   w = cosTheta;
   x = k * axis.x;
   y = k * axis.y;
   z = k * axis.z;
}

void
Quaternion::lookAt(const Point3f& from, const Point3f& target)
{
   // This subtraction is backwards from the way it ought to be because of opengl's
   // right-handedness.  I think.  IT WORKS--DON'T TOUCH!
   Point3f forward = (from - target);

   forward.Normalize();
   Point3f up(0, 1, 0);
   Point3f right = up.Cross(forward);
   right.Normalize();
   up = forward.Cross(right);
   up.Normalize();
   double trace = right.x + up.y + forward.z;

   if (trace > 0) {
      double s = sqrt(trace + 1.0) * 2.0;
      double s_recip = 1.0 / s;
	   w = 0.25 * s;
	   x = (up.z - forward.y) * s_recip;
	   y = (forward.x - right.z) * s_recip;
	   z = (right.y - up.x) * s_recip;
   } else if (right.x > up.y && right.x > forward.z) {
      double s = sqrt(1.0 + right.x - up.y - forward.z) * 2.0;
      double s_recip = 1.0 / s;
	   w = (up.z - forward.y) * s_recip;
	   x = 0.25 * s;
	   y = (up.x + right.y) * s_recip;
	   z = (forward.x + right.z) * s_recip;
   } else if (up.y > forward.z) {
      double s = sqrt(1.0 + up.y - right.x - forward.z) * 2.0;
      double s_recip = 1.0 / s;
	   w = (forward.x - right.z) * s_recip;
	   x = (up.x + right.y) * s_recip;
	   y = 0.25 * s;
	   z = (forward.y + up.z) * s_recip;
   } else {
      double s = sqrt(1.0 + forward.z - right.x - up.y) * 2.0;
      double s_recip = 1.0 / s;
	   w = (right.y - up.x) * s_recip;
	   x = (forward.x + right.z) * s_recip;
	   y = (forward.y + up.z) * s_recip;
	   z = 0.25 * s;
   }
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
    if (w <= DBL_EPSILON)
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
Quaternion::set(double z_rotation, double y_rotation, double x_rotation) 
{
    z_rotation *= 0.5f;
    y_rotation *= 0.5f;
    x_rotation *= 0.5f;

    // get sines and cosines of half angles
    double Cx, Sx;
    csg::SinCos(x_rotation, Sx, Cx);

    double Cy, Sy;
    csg::SinCos(y_rotation, Sy, Cy);

    double Cz, Sz;
    csg::SinCos(z_rotation, Sz, Cz);

    // multiply it out
    w = Cx*Cy*Cz - Sx*Sy*Sz;
    x = Sx*Cy*Cz + Cx*Sy*Sz;
    y = Cx*Sy*Cz - Sx*Cy*Sz;
    z = Cx*Cy*Sz + Sx*Sy*Cx;

}

void
Quaternion::get_axis_angle(Point3f& axis, double& angle) const
{
   // recall the rotation angle = 2*theta
   angle = 2.0f * std::acos(w);

   // Don't do this:
   // double length = sqrt(1.0f - w*w);

   // This is far more accurate:
   double length = std::sqrt(x*x + y*y + z*z);

   // For small angles, w is close to 1 and has poor precision because we spend all of our mantissa bits storing 9's
   // e.g. if w^2 = 0.999999, 1 - w^2 = 0.0000001?????
   // e.g. x^2 + y^2 + z^2 = 0.000000123123 has far more precision because the 6 leading 0's are omitted from the mantissa and stored in the exponent
   // (recall that a unit quaternion has 1 = w^2 + x^2 + y^2 + z^2)

   if (csg::IsZero(length)) {
      axis.SetZero();
   } else {
      double k = 1.0f / length;
      axis = Point3f(x*k, y*k, z*k);
   }
}

Point3f Quaternion::GetEulerAngle() const
{
    double test = x * y + z * w;
    if (test > 0.499) { // Singularity at north pole
        return Point3f(2 * atan2(x, w), 0, csg::k_pi/2);
    }
    if (test < -0.499) { // Singularity at south pole
        return Point3f(-2 * atan2(x, w), 0, -csg::k_pi/2);
    }

    double pitch, roll, yaw;
    yaw = atan2(2*(w*y + x*z), 1 - 2*(y*y + z*z)); // heading
    pitch = atan2(2*(w*x - z*y), 1 - 2*(x*x + z*z)); //bank
    roll = asin(2*(x*y + z*w)); // attitude

    return Point3f(pitch, yaw, roll);
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
    double lengthsq = w*w + x*x + y*y + z*z;

    if (csg::IsZero(lengthsq))
    {
        SetZero();
    }
    else
    {
        double factor = 1 / std::sqrt(lengthsq);
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
    double magnitude = quat.w*quat.w + quat.x*quat.x + quat.y*quat.y + quat.z*quat.z;
    // if we're the zero Quaternion, just return identity
    if (!csg::IsZero(magnitude))
    {
        ASSERT(false);
        return Quaternion();
    }

    double normRecip = 1.0f / magnitude;
    return Quaternion(normRecip*quat.w, -normRecip*quat.x, -normRecip*quat.y, 
                   -normRecip*quat.z);

}

const Quaternion& 
Quaternion::inverse()
{
    double magnitude = w*w + x*x + y*y + z*z;
    // if we're the zero Quaternion, just return
    if (csg::IsZero(magnitude))
        return *this;

    double normRecip = 1.0f / magnitude;
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
Quaternion::operator*=(double scalar)
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
double               
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

    double v_mult = 2.0f*(x*vector.x + y*vector.y + z*vector.z);
    double cross_mult = 2.0f*w;
    double p_mult = cross_mult*w - 1.0f;

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
csg::lerp(Quaternion& result, const Quaternion& start, const Quaternion& end, double t)
{
    // get cos of "angle" between Quaternions
    double cosTheta = start.Dot(end);

    // initialize result
    result = t*end;

    // if "angle" between Quaternions is less than 90 degrees
    if (cosTheta >= DBL_EPSILON)
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
csg::slerp(Quaternion& result, const Quaternion& start, const Quaternion& end, double t)
{
   // get cosine of "angle" between Quaternions
   double cosTheta = start.Dot(end);
   double startInterp, endInterp;

   // if "angle" between Quaternions is less than 90 degrees
   if (cosTheta >= DBL_EPSILON)
   {
      // if angle is greater than zero
      if ((1.0f - cosTheta) > DBL_EPSILON)
      {
         // use standard slerp
         double theta = acos(cosTheta);
         double recipSinTheta = 1.0f/std::sin(theta);

         startInterp = std::sin((1.0f - t)*theta)*recipSinTheta;
         endInterp = std::sin(t*theta)*recipSinTheta;
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
      if ((1.0f + cosTheta) > DBL_EPSILON)
      {
         // use slerp w/negation of start Quaternion
         double theta = acos(-cosTheta);
         double recipSinTheta = 1.0f/std::sin(theta);

         startInterp = std::sin((t-1.0f)*theta)*recipSinTheta;
         endInterp = std::sin(t*theta)*recipSinTheta;
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
csg::approx_slerp(Quaternion& result, const Quaternion& start, const Quaternion& end, double t)
{
    double cosTheta = start.Dot(end);

    // correct time by using cosine of angle between Quaternions
    double factor = 1.0f - 0.7878088f*cosTheta;
    double k = 0.5069269f;
    factor *= factor;
    k *= factor;

    double b = 2*k;
    double c = -3*k;
    double d = 1 + k;

    t = t*(b*t + c) + d;

    // initialize result
    result = t*end;

    // if "angle" between Quaternions is less than 90 degrees
    if (cosTheta >= DBL_EPSILON)
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


void Quaternion::SaveValue(protocol::quaternion *msg) const {
   msg->set_x(x);
   msg->set_y(y);
   msg->set_z(z);
   msg->set_w(w);
}

void Quaternion::LoadValue(const protocol::quaternion &msg) {
   x = msg.x();
   y = msg.y();
   z = msg.z();
   w = msg.w();
}

