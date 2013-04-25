#include "pch.h"
#include "skeleton.h"
#include "renderer.h"

using radiant::math3d::point3;
using radiant::client::Renderer;
using radiant::client::Skeleton;

Skeleton::Skeleton() :
   _parent(0)
{
}

Skeleton::~Skeleton()
{
   Clear();
   ASSERT(_bones.size() == 0);
}

void Skeleton::Clear()
{
   for (auto &entry : _bones) {
      if (entry.second) {
         h3dRemoveNode(entry.second);
      }
   }
   _bones.clear();
}

H3DNode Skeleton::AttachEntityToBone(H3DRes res, std::string bone, point3 offset)
{
   ASSERT(_parent);

   H3DNode b = _bones[bone];
   if (!b) {
      b = CreateBone(bone);
   }
   if (!offset.is_zero()) {
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

H3DNode Skeleton::GetSceneNode(std::string bone)
{
   ASSERT(_parent);

   H3DNode node = _bones[bone];
   if (!node) {
      node = CreateBone(bone);
   }
   return node;
}

H3DNode Skeleton::CreateBone(std::string bone)
{
   ostringstream name;
   name << "Skeleton " << _parent << " " + bone + " bone";
   //H3DNode scaler = h3dAddGroupNode(_parent, name.str().c_str());
   //h3dSetNodeTransform(scaler, 0, 0, 0, 0, 0, 0, .1f, .1f, .1f);

   name << "...";
   H3DNode b = h3dAddGroupNode(_parent, name.str().c_str());
   _bones[bone] = b;

   return b;
}
