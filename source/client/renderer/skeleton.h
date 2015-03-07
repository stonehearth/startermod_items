#ifndef _RADIANT_CLIENT_SKELETON_H
#define _RADIANT_CLIENT_SKELETON_H

#include "namespace.h"
#include "csg/point.h"
#include "h3d_resource_types.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class RenderEntity;

class Skeleton {
   public:
      Skeleton(RenderEntity& re);
      ~Skeleton();

      void SetSceneNode(H3DNode parent);
      void SetBoneVisible(std::string const& bone, bool visible);
      H3DNode GetSceneNode(std::string const& bone);
      int GetBoneNumber(std::string const& bone);
      int GetNumBones() const;

      void Clear();
      void SetScale(float scale) { _scale = scale; }
      float GetScale() const { return _scale; }

   private:
      H3DNode CreateBone(std::string const& bone);

   protected:
      RenderEntity&                            _renderEntity;
      std::unordered_map<std::string, H3DNode> _bones;
      std::unordered_map<std::string, int>     _boneNumLookup;
      float                                    _scale;
      std::unordered_map<std::string, int>     _visibleCount;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_SKELETON_H
 