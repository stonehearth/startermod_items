#include "pch.h"
#include "skeleton.h"
#include "renderer.h"
#include "render_entity.h"

using namespace ::radiant;
using namespace ::radiant::client;

std::string _default;

Skeleton::Skeleton(RenderEntity& re) :
   _renderEntity(re),
   _scale(1.0f)
{
}

Skeleton::~Skeleton()
{
   Clear();
   ASSERT(_bones.size() == 0);
}

void Skeleton::Clear()
{
   for (auto const& entry : _bones) {
      h3dRemoveNode(entry.second);
   }
   _bones.clear();
   _boneNumLookup.clear();
   _visibleCount.clear();
}


H3DNode Skeleton::GetSceneNode(std::string const& bone)
{
   auto& i = _bones.find(bone);
   if (i == _bones.end()) {
      CreateBone(bone);
      i = _bones.find(bone);
   }
   return i->second;
}

int Skeleton::GetNumBones() const
{
   return (int)_bones.size();
}

int Skeleton::GetBoneNumber(std::string const& bone)
{
   auto& i = _boneNumLookup.find(bone);
   if (i == _boneNumLookup.end()) {
      CreateBone(bone);
      i = _boneNumLookup.find(bone);
   }
   return i->second;
}

H3DNode Skeleton::CreateBone(std::string const& bone)
{
   H3DNode parent = _renderEntity.GetNode();
   std::ostringstream name;
   name << "Skeleton " << parent << " " << bone << " bone";

   int newBoneNum = GetNumBones();
   H3DNode b = h3dAddVoxelJointNode(parent, name.str().c_str(), newBoneNum);
   h3dSetNodeTransform(b, 0, 0, 0, 0, 0, 0, 1.0f, 1.0f, 1.0f);
   _boneNumLookup[bone] = newBoneNum;
   _bones[bone] = b;
   _visibleCount[bone] = 1;
   return b;
}

/*
 * Skeleton::SetBoneVisible --
 *
 * Many clients at many different times may want to toggle the visibility of the bones,
 * so keep a counter going.
 *
 */
void Skeleton::SetBoneVisible(std::string const& bone, bool visible)
{
   H3DNode node = GetSceneNode(bone);
   auto i = _visibleCount.find(bone);
   ASSERT(i != _visibleCount.end());

   if (visible) {
      i->second++;
   } else {
      i->second--;
   }
   if (i->second > 0) {
      h3dTwiddleNodeFlags(node, H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, false, false);
   } else {
      h3dTwiddleNodeFlags(node, H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, true, false);
   }
}

void Skeleton::SetScale(float scale)
{
   if (scale != _scale) {
      float ratio = scale / _scale;

      _scale = scale;

      for (auto const& entry : _bones) {
         float rel[16];
         const float *oldrel;
         H3DNode node = entry.second;

         h3dGetNodeTransMats(node, &oldrel, nullptr);
         memcpy(rel, oldrel, sizeof(rel));
         rel[12] *= ratio;
         rel[13] *= ratio;
         rel[14] *= ratio;
         h3dSetNodeTransMat(node, rel);
      }
   }
}
