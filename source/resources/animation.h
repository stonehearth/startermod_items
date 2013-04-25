#ifndef _RADIANT_RESOURCES_ANIMATION_H
#define _RADIANT_RESOURCES_ANIMATION_H

#include "math3d.h"
#include "resource.h"
#include "libjson.h"

BEGIN_RADIANT_RESOURCES_NAMESPACE

class Animation : public Resource {
public:

   static std::string JsonToBinary(const JSONNode &obj);

   Animation(std::string name, std::string bin);

   static ResourceType Type;
   ResourceType GetType() const override { return Resource::ANIMATION; }

   float GetDuration();

   typedef std::function<void(std::string, const math3d::transform &)> AnimationFn;
   void MoveNodes(int32 timeOffset, AnimationFn fn);
   bool HasBone(std::string bone);

protected:
   struct BinaryHeader {
      uint32         numBones;
      uint32         numFrames;
      uint32         framesPerSecond;
      uint32         firstFrameOffset;
   };

   struct Bone {
      float                position[3];
      float                quaternion[4];
   };

   struct Frame {
      std::map<std::string, Bone>   bones;
   };

protected:
   void MoveNodes(float offset, AnimationFn fn);
   void GetFrames(float offset, int &f0, int &f1, float &alpha);

private:
   std::string         _name;
   std::string    bin_;
   const char     *_base;
   int            _len;
};

std::ostream& operator<<(std::ostream& out, const Animation& source);

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_ANIMATION_H
