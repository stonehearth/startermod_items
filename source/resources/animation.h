#ifndef _RADIANT_RES_ANIMATION_H
#define _RADIANT_RES_ANIMATION_H

#include <functional>
#include "libjson.h"
#include "namespace.h"
#include "csg/transform.h"

BEGIN_RADIANT_RES_NAMESPACE

class Animation {
public:
   Animation(std::string bin);
   static std::string JsonToBinary(const JSONNode &obj);

   float GetDuration();

   typedef std::function<void(std::string, const csg::Transform &)> AnimationFn;
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
   std::string    bin_;
   const char     *_base;
   int            _len;
};

typedef std::shared_ptr<Animation> AnimationPtr;

std::ostream& operator<<(std::ostream& out, const Animation& source);

END_RADIANT_RES_NAMESPACE

#endif //  _RADIANT_RES_ANIMATION_H
