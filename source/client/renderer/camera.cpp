#include "pch.h"
#include "camera.h"
#include "Horde3D.h"
#include "csg/transform.h"
#include "csg/ray.h"
#include "csg/matrix.h"

using namespace ::radiant;
using namespace ::radiant::client;


Camera::Camera(H3DNode parent, const char* name,H3DRes pipeline) :
   node_(h3dAddCameraNode(parent, name, pipeline))
{
   
}

// Orbit the specified point, first by 'yDeg' along the world-space y-axis, and then 'xDeg' along
// the camera's x-axis (the axis point orthogonally to both 'forward' and 'up').  The angle of
// movement around the x-axis is bound between minX and maxX.
void Camera::OrbitPointBy(const csg::Point3f &point, float xDeg, float yDeg, float minX, float maxX) 
{
   float degToRad = csg::k_pi / 180.0f;

   csg::Point3f originVec = GetPosition() - point;
   csg::Point3f newPosition = originVec;
   originVec.Normalize();
   float xAng = acos(originVec.y) / degToRad;

   if (xAng + xDeg > maxX) {
      xDeg = maxX - xAng;
   } else if (xAng + xDeg < minX) {
      xDeg = minX - xAng;
   }

   csg::Point3f forward, up, left;
   GetBases(&forward, &up, &left);
   csg::Quaternion yRot(csg::Point3f(0, 1, 0), yDeg * degToRad);
   csg::Quaternion xRot(left, xDeg * degToRad);

   newPosition = xRot.rotate(newPosition);
   newPosition = yRot.rotate(newPosition);
   SetPosition(newPosition + point);
}

void Camera::LookAt(const csg::Point3f &point)
{
   csg::Point3f forward = GetPosition() - point;
   forward.Normalize();

   // We take a pretend up to get us a real left, and then cross again to get the actual up.
   csg::Point3f up(0, 1, 0);
   csg::Point3f left = up.Cross(forward);
   left.Normalize();
   up = forward.Cross(left);
   up.Normalize();

   SetBases(forward, up, left);
}

H3DNode Camera::GetNode() const
{
   return node_.get();
}

csg::Matrix4 Camera::GetMatrix() const
{
   csg::Matrix4 m;
   const float *f;
   h3dGetNodeTransMats(node_.get(), NULL, &f);
   m.fill(f);
   return m;
}

csg::Point3f Camera::GetPosition() const
{
   csg::Matrix4 m = GetMatrix();

   return m.get_translation();
}

void Camera::SetOrientation(const csg::Quaternion& quat)
{
   const csg::Point3f trans = GetMatrix().get_translation();
   
   csg::Matrix4 m = GetMatrix().rotation(quat);
   m.set_translation(trans);
   
   h3dSetNodeTransMat(node_.get(), m.get_float_ptr());
}

csg::Quaternion Camera::GetOrientation() const
{
   return csg::Quaternion(GetMatrix());
}

void Camera::SetPosition(const csg::Point3f &newPos)
{
   csg::Matrix4 m = GetMatrix();

   m.set_translation(newPos);
   h3dSetNodeTransMat(node_.get(), m.get_float_ptr());
}

void Camera::GetBases(csg::Point3f* const forward, csg::Point3f* const up, csg::Point3f* const left) const
{
   const float *f;
   h3dGetNodeTransMats(node_.get(), NULL, &f);

   left->x = f[0];
   left->y = f[1];
   left->z = f[2];

   up->x = f[4];
   up->y = f[5];
   up->z = f[6];

   forward->x = f[8];
   forward->y = f[9];
   forward->z = f[10];
}

void Camera::SetBases(const csg::Point3f& forward, const csg::Point3f& up, const csg::Point3f& left)
{
   csg::Matrix4 m = GetMatrix();
   m.set_rotation_bases(forward, up, left);
   h3dSetNodeTransMat(node_.get(), m.get_float_ptr());   
}

Camera::~Camera()
{
}
