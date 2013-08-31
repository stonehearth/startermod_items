#include "pch.h"
#include "camera.h"
#include "Horde3D.h"
#include "csg/transform.h"
#include "csg/ray.h"
#include "csg/matrix.h"

using namespace ::radiant;
using namespace ::radiant::client;


Camera::Camera(H3DNode cameraNode) :
   node_(cameraNode)
{
}

void Camera::OrbitPointBy(const csg::Point3f &point, float xDeg, float yDeg, float minX, float maxX) 
{
   float x, y, z;
   float degToRad = csg::k_pi / 180.0f;
   GetMatrix().get_fixed_angles(z, y, x);

   csg::Point3f newPosition = GetPosition() - point;

   csg::Matrix3 yRot, xRot;
   yRot.rotation_y(yDeg * degToRad);
   xRot.rotation_x(xDeg * degToRad);

   csg::Matrix3 viewRot;
   viewRot.rotation(z, y, x);

   // Undo the current camera's rotation; then, apply the orbit; then, redo the camera's rotation.
   // STILL clearer than bloody quaternions.
   newPosition = viewRot.inverse() * newPosition;
   newPosition = yRot * xRot * newPosition;
   newPosition = viewRot.inverse() * newPosition;

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
   return node_;
}

csg::Matrix4 Camera::GetMatrix() const
{
   csg::Matrix4 m;
   const float *f;
   h3dGetNodeTransMats(node_, NULL, &f);
   m.fill(f);
   return m;
}

csg::Point3f Camera::GetPosition() const
{
   csg::Matrix4 m = GetMatrix();

   return m.get_translation();
}

void Camera::SetPosition(const csg::Point3f &newPos)
{
   csg::Matrix4 m = GetMatrix();

   m.set_translation(newPos);
   h3dSetNodeTransMat(node_, m.get_float_ptr());
}

void Camera::GetBases(csg::Point3f * const forward, csg::Point3f* const up, csg::Point3f* const left) const
{
   const float *f;
   h3dGetNodeTransMats(node_, NULL, &f);

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
   h3dSetNodeTransMat(node_, m.get_float_ptr());   
}

Camera::~Camera()
{
}
