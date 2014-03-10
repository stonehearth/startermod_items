#include "pch.h"
#include "skeleton.h"
#include "renderer.h"

using namespace ::radiant;
using radiant::csg::Point3f;
using radiant::client::Renderer;
using radiant::client::Skeleton;

Skeleton::Skeleton() :
   _parent(0),
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
   _bones.clear();
}

H3DNode Skeleton::AttachEntityToBone(H3DRes res, const std::string& bone, csg::Point3f const& offset)
{
   ASSERT(_parent);

   H3DNode b = _bones[bone].get();
   if (!b) {
      b = CreateBone(bone);
   }
   if (offset.x || offset.y || offset.z) {
      assert(false);
      // b = b->createChildSceneNode(name, Vector3(offset));
   }
   return h3dAddNodes(b, res);
}

void Skeleton::SetSceneNode(H3DNode parent)
{
   ASSERT(parent);
   ASSERT(!_parent || (_parent == parent));
   _parent = parent;
}


void Skeleton::ApplyScaleToBones()
{
   for (auto const& entry : _bones) {
      H3DNode bone = entry.second.get();
      float tx, ty, tz, rx, ry, rz, sx, sy, sz;

      h3dGetNodeTransform(bone, &tx, &ty, &tz, &rx, &ry, &rz, &sx, &sy, &sz);
      tx *= (_scale / sx);
      ty *= (_scale / sy);
      tz *= (_scale / sz);
      LOG_(0) << "setting scale of " << bone << " to " << _scale;
      h3dSetNodeTransform(bone, tx, ty, tz, rx, ry, rz, _scale, _scale, _scale);
   }
}

H3DNode Skeleton::GetSceneNode(const std::string& bone)
{
   ASSERT(_parent);

   H3DNode node = _bones[bone].get();
   if (!node) {
      node = CreateBone(bone);
   }
   return node;
}

H3DNode Skeleton::CreateBone(const std::string& bone)
{
   std::ostringstream name;
   name << "Skeleton " << _parent << " " + bone + " bone";
   //H3DNode scaler = h3dAddGroupNode(_parent, name.str().c_str());
   //h3dSetNodeTransform(scaler, 0, 0, 0, 0, 0, 0, .1f, .1f, .1f);

   name << "...";
   H3DNode b = h3dAddGroupNode(_parent, name.str().c_str());
   _bones[bone] = H3DNodeUnique(b);

   return b;
}
