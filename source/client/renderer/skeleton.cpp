#include "pch.h"
#include "skeleton.h"
#include "renderer.h"
#include "render_entity.h"

using namespace ::radiant;
using namespace ::radiant::client;

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
}

H3DNode Skeleton::AttachEntityToBone(H3DRes res, std::string const& bone, csg::Point3f const& offset)
{
   H3DNode b = _bones[bone];
   if (!b) {
      b = CreateBone(bone);
   }
   if (offset.x || offset.y || offset.z) {
      assert(false);
      // b = b->createChildSceneNode(name, Vector3(offset));
   }
   return h3dAddNodes(b, res);
}

void Skeleton::ApplyScaleToBones()
{
   for (auto const& entry : _bones) {
      H3DNode bone = entry.second;
      float tx, ty, tz, rx, ry, rz, sx, sy, sz;

      h3dGetNodeTransform(bone, &tx, &ty, &tz, &rx, &ry, &rz, &sx, &sy, &sz);
      tx *= (_scale / sx);
      ty *= (_scale / sy);
      tz *= (_scale / sz);
      h3dSetNodeTransform(bone, tx, ty, tz, rx, ry, rz, _scale, _scale, _scale);
   }
}

H3DNode Skeleton::GetSceneNode(std::string const& bone)
{
   H3DNode node = _bones[bone];
   if (!node) {
      node = CreateBone(bone);
   }
   return node;
}

H3DNode Skeleton::CreateBone(std::string const& bone)
{
   H3DNode parent = _renderEntity.GetNode();
   std::ostringstream name;
   name << "Skeleton " << parent << " " + bone + " bone";
   //H3DNode scaler = h3dAddGroupNode(_parent, name.str().c_str());
   //h3dSetNodeTransform(scaler, 0, 0, 0, 0, 0, 0, .1f, .1f, .1f);

   name << "...";
   H3DNode b = h3dAddGroupNode(parent, name.str().c_str());
   h3dSetNodeTransform(b, 0, 0, 0, 0, 0, 0, 1.0f, 1.0f, 1.0f);
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
   if (visible > 0) {
      h3dTwiddleNodeFlags(node, H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, false, false);
   } else {
      h3dTwiddleNodeFlags(node, H3DNodeFlags::NoDraw | H3DNodeFlags::NoRayQuery, true, false);
   }
}

