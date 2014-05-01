#ifndef _RADIANT_CLIENT_SKELETON_H
#define _RADIANT_CLIENT_SKELETON_H

#include "namespace.h"
#include "csg/point.h"
#include "h3d_resource_types.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Skeleton {
   public:
      Skeleton();
      ~Skeleton();

      void SetSceneNode(H3DNode parent);
      H3DNode GetSceneNode(std::string const& bone);

      H3DNode AttachEntityToBone(H3DRes entity, std::string const& bone, csg::Point3f const& offset);
      void Clear();
      void SetScale(float scale) { _scale = scale; }
      float GetScale() const { return _scale; }
      void ApplyScaleToBones();
      H3DNode GetRootNode() const { return _parent; }

   private:
      H3DNode CreateBone(std::string const& bone);

   protected:
      H3DNode                                _parent;
      std::map<std::string, H3DNodeUnique>   _bones;
      float                                  _scale;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_SKELETON_H
 