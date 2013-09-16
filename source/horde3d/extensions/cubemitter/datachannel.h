#ifndef _RADIANT_HORDE3D_EXTENSIONS_DATACHANNEL_H
#define _RADIANT_HORDE3D_EXTENSIONS_DATACHANNEL_H

#include "egPrimitives.h"
#include "namespace.h"

#include "libjson.h"
#include "radiant_json.h"

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

private:
   Vec3f _small, _big;
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
            return (1.0 - frac) * oldVal + (frac * val.second);
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

private:
   const std::vector<std::pair<float, float> > _curveValues;
};

class RandomBetweenLinearCurvesValueEmitter : public CurveValueEmitter
{
public:
   RandomBetweenLinearCurvesValueEmitter(
      const std::vector<std::pair<float, float> > &bottomValues,
      const std::vector<std::pair<float, float> > &topValues) : _topValues(topValues), _bottomValues(bottomValues)
   {
      _times = collectTimes(_bottomValues, _topValues);
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

private:

   std::vector<float> collectTimes (const std::vector<std::pair<float, float> > &curve1, 
                                               const std::vector<std::pair<float, float> > &curve2)
   {
      std::vector<float> result;

      if (curve1.size() == 0) 
      {
         if (curve2.size() == 0) 
         {
            return result;
         }
         //result.insert(result.end(), curve2.begin(), curve2.end());
         return result;
      } else if (curve2.size() == 0) {
         //result.insert(result.end(), curve1.begin(), curve1.end());
         return result;
      }

      std::pair<float, float> v1;
      std::pair<float, float> v2;
      int i1 = 0, i2 = 0;

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
   std::vector<std::pair<float, float> > _randomValues;
   std::vector<float> _times;
};


template<typename T> class DataChannel
{
public:
   virtual ~DataChannel() 
   {
   }

   void init() 
   {
   }

   T nextValue(float time) 
   {
      return 0.0f;
   }
};

template<> 
class DataChannel<Vec3f>
{
public:
   DataChannel(const Vec3f& val)
   {
      _v = new ConstantValueEmitter<Vec3f>(val);
   }

   DataChannel(const Vec3f& min, const Vec3f& max)
   {
      _v = new RandomBetweenVec3fEmitter(min, max);
   }

   virtual ~DataChannel() 
   {
      delete _v;
   }

   void init() 
   {
      _v->init();
   }

   Vec3f nextValue(float time) 
   {
      return _v->nextValue(time);
   }

private:
   ValueEmitter<Vec3f> *_v;
};

template<> 
class DataChannel<float>
{
public:
   DataChannel(float constantValue)
   {
      _v = new ConstantValueEmitter<float>(constantValue);
   }

   DataChannel(float minV, float maxV)
   {
      _v = new RandomBetweenValueEmitter(minV, maxV);
   }

   DataChannel(const std::vector<std::pair<float, float> > &curveVals)
   {
      _v = new LinearCurveValueEmitter(curveVals);
   }

   DataChannel(
      const std::vector<std::pair<float, float> > &bottomCurve, 
      const std::vector<std::pair<float, float> > &topCurve)
   {
      _v = new RandomBetweenLinearCurvesValueEmitter(bottomCurve, topCurve);
   }

   virtual ~DataChannel() 
   {
      delete _v;
      _v = nullptr;
   }

   void init() 
   {
      _v->init();
   }

   float nextValue(float time) 
   {
      return _v->nextValue(time);
   }

private:
   ValueEmitter<float> *_v;
};

END_RADIANT_HORDE3D_NAMESPACE

#endif // _RADIANT_HORDE3D_EXTENSIONS_DATACHANNEL_H
