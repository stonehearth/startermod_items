#ifndef _RADIANT_CLIENT_RENDERER_CAMERA_H
#define _RADIANT_CLIENT_RENDERER_CAMERA_H

#include "namespace.h"
#include "Horde3D.h"
#include "csg/transform.h"
#include "csg/ray.h"
#include "csg/matrix.h"
#include "h3d_resource_types.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Camera
{
   public:
      Camera(H3DNode parent, const char* name,H3DRes pipeline);
      virtual ~Camera();

      void OrbitPointBy(const csg::Point3f &point, float xRot, float yRot, float maxX, float minX);
      void LookAt(const csg::Point3f &point);

      H3DNode GetNode() const;
      //csg::Quaternion GetRotation() const;
      csg::Point3f GetPosition() const;
      void SetPosition(const csg::Point3f &newPos);


      void Camera::SetBases(const csg::Point3f& forward, const csg::Point3f& up, const csg::Point3f& left);
      void GetBases(csg::Point3f* const forward, csg::Point3f* const up, csg::Point3f* const left) const;

   private:
      NO_COPY_CONSTRUCTOR(Camera);

   private:
      csg::Matrix4 GetMatrix() const;

      const H3DNodeUnique node_;
};

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_RENDERER_CAMERA_H