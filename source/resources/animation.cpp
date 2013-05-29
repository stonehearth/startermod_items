#include "radiant.h"
#include "animation.h"
#include "exceptions.h"
#include "radiant_luabind.h"

static const float FRAME_DURATION_MS = (1000.0f / 30.0f);

using namespace ::radiant;
using namespace ::radiant::resources;

using ::radiant::int32;
using ::radiant::math3d::transform;

Animation::Animation(std::string bin) :
   bin_(bin),
   _base(&bin_[0]),
   _len(bin_.size())
{
}


void Animation::RegisterType(lua_State* L)
{
   using namespace luabind;

   module(L) [
      class_<Animation, AnimationPtr>("Animation")
         .def(tostring(self))
         .def("get_duration", &resources::Animation::GetDuration)
   ];
}

std::string Animation::JsonToBinary(const JSONNode &node)
{
   const JSONNode &frames = node["frames"];
   const JSONNode &firstFrame = *frames.begin();
   std::vector<char> result(sizeof(BinaryHeader));

   BinaryHeader *header = (BinaryHeader *)&result[0];

   header->numFrames = frames.size();
   header->numBones = firstFrame.size();
   header->framesPerSecond = 30;

   // copy the bone names...
   for (auto i = firstFrame.begin(); i != firstFrame.end(); i++) {
      std::string name = i->name();
      result.insert(result.end(), name.begin(), name.end());
      result.push_back(0);
   }
   result.push_back(0);
   header = (BinaryHeader *)&result[0];
   header->firstFrameOffset = result.size();

   // copy the frames
   for (auto i = frames.begin(); i != frames.end(); i++) {
      for (auto j = (*i).begin(); j != (*i).end(); j++) {
         const JSONNode &pos = (*j)["pos"];
         const JSONNode &rot = (*j)["rot"];

#if 0
         float value;
         for (int i = 0; i < 3; i++) {
            value = (float)pos[i].as_float() / 10.0f;
            result.insert(result.end(), (char *)&value, (char *)(&value + 1));
         }
         for (int i = 0; i < 4; i++) {
            value = (float)rot[i].as_float();
            result.insert(result.end(), (char *)&value, (char *)(&value + 1));
         }
#else
         float quat[4], q[4], trans[3], t[3];
         for (int i = 0; i < 3; i++) {
            t[i] = (float)pos[i].as_float();
         }
         for (int i = 0; i < 4; i++) {
            q[i] = (float)rot[i].as_float();
         }
         // convert from z-up to y-up
         trans[0] = t[0];
         trans[1] = t[2];
         trans[2] = -t[1];

         // flip -y and z to convert from z-up to y-up
         quat[0] = q[0];
         quat[1] = q[1];
         quat[2] = q[3];
         quat[3] = -q[2];
         result.insert(result.end(), (char *)trans, (char *)(trans + 3));
         result.insert(result.end(), (char *)quat, (char *)(quat + 4));
#endif
      }
   }
   return std::string(&result[0], result.size());
}

float Animation::GetDuration()
{

   BinaryHeader *header = (BinaryHeader *)_base;
   return (header->numFrames - 1) * FRAME_DURATION_MS;
   //return ((float)header->numFrames) / header->framesPerSecond;
}

void Animation::MoveNodes(int32 timeOffset, AnimationFn fn)
{
   float duration = GetDuration();
   int32 loops = (int)(timeOffset / duration);
   int32 position = (int32)(timeOffset - (loops * duration));

   MoveNodes(position / duration, fn);  
}

void Animation::MoveNodes(float offset, AnimationFn fn)
{
   BinaryHeader *header = (BinaryHeader *)_base;
   float alpha;
   int f, n;

   GetFrames(offset, f, n, alpha);

#define FRAME(count) (float *)(_base + header->firstFrameOffset + ((count) * (header->numBones * sizeof(float) * 7)))

   //LOG(WARNING) << "Rendering animation " << _name << " offset " << offset << " frame " << f << " interpolated " << alpha << ".";

   char *boneptr = ((char *)(header + 1));
   float *first = FRAME(f);
   float *next = FRAME(n);

   while (*boneptr) {
      std::string bone(boneptr);
      boneptr += bone.size() + 1;

      math3d::transform b0(math3d::point3(first[0], first[1], first[2]),
                           math3d::quaternion(first[3], first[4], first[5], first[6]));
      math3d::transform b1(math3d::point3(next[0], next[1], next[2]),
                           math3d::quaternion(next[3], next[4], next[5], next[6]));

      //LOG(WARNING) << _name << " " << f << " " << bone << " " << "b0: " << b0.position;
      //LOG(WARNING) << _name << " " << n << " " << bone << " " << "b1: " << b1.position;

      math3d::transform t = math3d::interpolate(b0, b1, alpha);
      //t.orientation = math3d::quaternion(math3d::vector3::unit_y, 0);
      fn(bone, t);

      first += 7;
      next += 7;

      ASSERT(((char *)first - _base) <= _len);
      ASSERT(((char *)next - _base) <= _len);
   }
   //LOG(WARNING) << "---";
}

void Animation::GetFrames(float offset, int &f0, int &f1, float &alpha)
{
   BinaryHeader *header = (BinaryHeader *)_base;

   static volatile int pause = false;
   static volatile int which = 0;
   if (pause) {
      f0 = f1 = which;
      alpha = 0.0f;
      return;
   }

   if (offset <= 0) {
      f0 = f1 = 0;
      alpha = 0.0f;
   } else if (offset >= 1.0f) {
      f0 = f1 = header->numFrames - 1;
      alpha = 0.0f;
   } else {
      float current = (header->numFrames - 1) * offset;
      f0 = (int)current;
      f1 = f0 + 1;

      ASSERT((unsigned int)f1 < header->numFrames);

      alpha = current - f0;
   }
}

bool Animation::HasBone(std::string name)
{
   BinaryHeader *header = (BinaryHeader *)_base;
   char *boneptr = ((char *)(header + 1));

   while (*boneptr) {
      std::string bone(boneptr);
      if (bone == name) {
         return true;
      }
      boneptr += bone.size() + 1;
   }
   return false;
}

std::ostream& ::radiant::resources::operator<<(std::ostream& out, const Animation& source) {
   return (out << "[Animation]");
}
