#include "pch.h"
#include "random_number_generator.h"
#include <random>
#include <chrono>
#include <type_traits>

using namespace ::radiant;
using namespace ::radiant::csg;

RandomNumberGenerator::RandomNumberGenerator()
{
   unsigned int seed = std::chrono::high_resolution_clock::now().time_since_epoch().count() % ULONG_MAX;
   SetSeed(seed);
}

RandomNumberGenerator::RandomNumberGenerator(unsigned int seed)
{
   SetSeed(seed);
}

void RandomNumberGenerator::SetSeed(unsigned int seed)
{
   generator_ = std::default_random_engine(seed);
}

template <class T>
T RandomNumberGenerator::GenerateUniformInt(T min, T max)
{
   static_assert(std::is_integral<T>::value, "<T> must be an integral type");
   ASSERT(min <= max);

   std::uniform_int_distribution<T> distribution(min, max);
   return distribution(generator_);
}

template <class T>
T RandomNumberGenerator::GenerateUniformReal(T min, T max)
{
   static_assert(std::is_floating_point<T>::value, "<T> must be a floating point type");
   ASSERT(min <= max);

   std::uniform_real_distribution<T> distribution(min, max);
   return distribution(generator_);
}

template <class T>
T RandomNumberGenerator::GenerateGaussian(T mean, T std_dev)
{
   static_assert(std::is_floating_point<T>::value, "<T> must be a floating point type");

   std::normal_distribution<T> distribution(mean, std_dev);
   return distribution(generator_);
}

std::ostream& csg::operator<<(std::ostream& out, const RandomNumberGenerator& source)
{
   out << "RandomNumberGenerator";
   return out;
}

#define MAKE_INT_METHODS(T) \
   template T RandomNumberGenerator::GenerateUniformInt(T min, T max);

#define MAKE_REAL_METHODS(T) \
   template T RandomNumberGenerator::GenerateUniformReal(T min, T max); \
   template T RandomNumberGenerator::GenerateGaussian(T mean, T std_dev);

MAKE_INT_METHODS(int)
MAKE_INT_METHODS(unsigned int)
MAKE_INT_METHODS(long long)
MAKE_INT_METHODS(unsigned long long)

MAKE_REAL_METHODS(float)
MAKE_REAL_METHODS(double)
