#ifndef _RADIANT_HORDE3D_EXTENSIONS_DATACHANNEL_H
#define _RADIANT_HORDE3D_EXTENSIONS_DATACHANNEL_H

#include "egPrimitives.h"
#include "namespace.h"
#include "radiant_json.h"

using namespace ::radiant;
using namespace ::radiant::json;

using namespace ::Horde3D;

BEGIN_RADIANT_HORDE3D_NAMESPACE

template<typename T> class ValueEmitter
{
public:
   virtual void init() = 0;
   virtual T nextValue(float time) = 0;

   float random(float min, float max)
   {
	   return (rand() / (float)RAND_MAX) * (max - min) + min;
   }

   virtual ValueEmitter<T>* clone() = 0;

};

template<typename T> class ConstantValueEmitter : public ValueEmitter<T>
{
public:
   ConstantValueEmitter(T value) : _value(value)
   {
   }

   void init() {}
   
   T nextValue(float time) 
   { 
      return _value; 
   }

   ConstantValueEmitter<T>* clone()
   {
      return new ConstantValueEmitter<T>(_value);
   }

private:
   T _value;
};

class RandomBetweenValueEmitter : public ValueEmitter<float>
{
public:
   RandomBetweenValueEmitter(float small, float big) : _small(small), _big(big)
   {
   }

   void init() {}
   
   float nextValue(float time)
   {
      return random(_small, _big);
   }

   RandomBetweenValueEmitter* clone()
   {
      return new RandomBetweenValueEmitter(_small, _big);
   }


private:
   float _small, _big;
};

class RandomBetweenVec3fEmitter : public ValueEmitter<Vec3f>
{
public:
   RandomBetweenVec3fEmitter(Vec3f small, Vec3f big) : _small(small), _big(big)
   {
      if (small.x > big.x) {
         float t = small.x;
         small.x = big.x;
         big.x = t;
      }
      if (small.y > big.y) {
         float t = small.y;
         small.y = big.y;
         big.y = t;
      }
      if (small.z > big.z) {
         float t = small.z;
         small.z = big.z;
         big.z = t;
      }
   }

   void init() {}
   
   Vec3f nextValue(float time)
   {
      return Vec3f(
         random(_small.x, _big.x),
         random(_small.y, _big.y),
         random(_small.z, _big.z));
   }

   RandomBetweenVec3fEmitter* clone()
   {
      return new RandomBetweenVec3fEmitter(_small, _big);
   }


private:
   Vec3f _small, _big;
};

class RandomBetweenVec4fEmitter : public ValueEmitter<Vec4f>
{
public:
   RandomBetweenVec4fEmitter(Vec4f small, Vec4f big) : _small(small), _big(big)
   {
      if (small.x > big.x) {
         float t = small.x;
         small.x = big.x;
         big.x = t;
      }
      if (small.y > big.y) {
         float t = small.y;
         small.y = big.y;
         big.y = t;
      }
      if (small.z > big.z) {
         float t = small.z;
         small.z = big.z;
         big.z = t;
      }
      if (small.w > big.w) {
         float t = small.w;
         small.w = big.w;
         big.w = t;
      }
   }

   void init() {}
   
   Vec4f nextValue(float time)
   {
      return Vec4f(
         random(_small.x, _big.x),
         random(_small.y, _big.y),
         random(_small.z, _big.z),
         random(_small.w, _big.w));
   }

   RandomBetweenVec4fEmitter* clone()
   {
      return new RandomBetweenVec4fEmitter(_small, _big);
   }


private:
   Vec4f _small, _big;
};



class CurveValueEmitter : public ValueEmitter<float>
{
public:
   CurveValueEmitter()
   {
   }

   float curveValueAt(float t, const std::vector<std::pair<float, float> > &curve)
   {
      if (curve.size() == 0) {
         return 0;
      }

      float oldT = curve[0].first;
      float oldVal = curve[0].second;

      for (const auto &val : curve)
      {
         if (t < val.first)
         {
            float frac = (t - oldT) / (val.first - oldT);
            return (1.0f - frac) * oldVal + (frac * val.second);
         }

         oldT = val.first;
         oldVal = val.second;
      }

      return curve[curve.size() - 1].second;
   }
};

class LinearCurveValueEmitter : public CurveValueEmitter
{
public:
   LinearCurveValueEmitter(const std::vector<std::pair<float, float> > &curveValues) : _curveValues(curveValues)
   {
   }

   void init() {}
   
   float nextValue(float time)
   {
      return curveValueAt(time, _curveValues);
   }

   LinearCurveValueEmitter* clone()
   {
      return new LinearCurveValueEmitter(_curveValues);
   }

private:
   const std::vector<std::pair<float, float> > _curveValues;
};

class RandomBetweenLinearCurvesValueEmitter : public CurveValueEmitter
{
public:
   RandomBetweenLinearCurvesValueEmitter(
      const std::vector<std::pair<float, float> > &bottomValues,
      const std::vector<std::pair<float, float> > &topValues) : 
         _topValues(topValues), _bottomValues(bottomValues), _times(collectTimes(bottomValues, topValues))
   {
      init();
   }

   void init()
   {
      _randomValues.clear();
      for (auto const &t : _times)
      {
         _randomValues.push_back(std::pair<float, float>(
            t, 
            random(curveValueAt(t, _bottomValues), curveValueAt(t, _topValues))));
      }
   }

   float nextValue(float time)
   {
      return curveValueAt(time, _randomValues);
   }

   RandomBetweenLinearCurvesValueEmitter* clone()
   {
      return new RandomBetweenLinearCurvesValueEmitter(_bottomValues, _topValues);
   }

private:

   std::vector<float> collectTimes (const std::vector<std::pair<float, float> > &curve1, 
                                               const std::vector<std::pair<float, float> > &curve2)
   {
      std::vector<float> result;
      std::pair<float, float> v1;
      std::pair<float, float> v2;

      if (curve1.size() == 0)
      {
         v1 = std::pair<float, float>(-1.0f, -1.0f);
      }
      if (curve2.size() == 0)
      {
         v2 = std::pair<float, float>(-1.0f, -1.0f);
      }
      unsigned int i1 = 0, i2 = 0;

      while (i1 < curve1.size() || i2 < curve2.size()) 
      {
         if (i1 < curve1.size()) 
         {
            v1 = curve1[i1];
         }
         if (i2 < curve2.size())
         {
            v2 = curve2[i2];
         }

         if (v1.first < v2.first)
         {
            result.push_back(v1.first);
            i1++;
         } else if (v1.first > v2.first) {
            result.push_back(v2.first);
            i2++;
         } else {
            result.push_back(v1.first);
            i1++;
            i2++;
         } 
      }

      return result;
   }

   const std::vector<std::pair<float, float> > _topValues;
   const std::vector<std::pair<float, float> > _bottomValues;
   const std::vector<float> _times;

   std::vector<std::pair<float, float> > _randomValues;
};

ValueEmitter<float>* parseChannel(ConstJsonObject &n, const char *childName, float def);

ValueEmitter<Vec3f>* parseChannel(ConstJsonObject &n, const char *childName, const Vec3f &def);

ValueEmitter<Vec4f>* parseChannel(ConstJsonObject &n, const char *childName, const Vec4f &def);

END_RADIANT_HORDE3D_NAMESPACE

#endif // _RADIANT_HORDE3D_EXTENSIONS_DATACHANNEL_H
