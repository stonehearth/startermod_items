#include "pch.h"
#include "random_number_generator.h"
#include <random>
#include <chrono>
#include <type_traits>

using namespace ::radiant;
using namespace ::radiant::csg;

std::map<boost::thread::id, std::unique_ptr<RandomNumberGenerator>> RandomNumberGenerator::instances_;
std::recursive_mutex RandomNumberGenerator::mutex_;

// Gets the default instance for this thread
// If you want to set the seed, you need to set it on the instance after the thread is created
RandomNumberGenerator& RandomNumberGenerator::DefaultInstance()
{
   std::lock_guard<std::recursive_mutex> lock(mutex_);
   boost::thread::id id = boost::this_thread::get_id();

   auto i = instances_.find(id);
   if (i != instances_.end()) {
      return *i->second;
   }

   instances_[id] = std::unique_ptr<RandomNumberGenerator>(new RandomNumberGenerator());
   return *instances_[id];
}

// Create a new RandomNumberGenerator seeded using the high resolution timer
RandomNumberGenerator::RandomNumberGenerator()
{
   // too heavyweight? could lazy generate the seed if none is set
   unsigned int seed = std::chrono::high_resolution_clock::now().time_since_epoch().count() % ULLONG_MAX;
   SetSeed(seed);
}

// Create a new RandomNumberGenerator seeded using the specified seed
RandomNumberGenerator::RandomNumberGenerator(unsigned int seed)
{
   SetSeed(seed);
}

// Seed can be set at any time. All subsequent numbers will start using the new seed.
void RandomNumberGenerator::SetSeed(unsigned int seed)
{
   if (seed == 0) {
      // some generators do not accept 0 as a seed
      seed = ULONG_MAX;
   }
   generator_ = std::default_random_engine(seed);
}

// Generates integers that are uniformly distributed over the interval [min, max] (min and max inclusive)
template <class T>
T RandomNumberGenerator::GetInt(T min, T max)
{
   static_assert(std::is_integral<T>::value, "<T> must be an integral type");
   ASSERT(min <= max);

   std::uniform_int_distribution<T> distribution(min, max);
   return distribution(generator_);
}

// Generates real numbers that are uniformly distributed over the interval [min, max) (includes min, excludes max)
template <class T>
T RandomNumberGenerator::GetReal(T min, T max)
{
   static_assert(std::is_floating_point<T>::value, "<T> must be a floating point type");
   ASSERT(min <= max);

   std::uniform_real_distribution<T> distribution(min, max);
   return distribution(generator_);
}

// Generates real numbers that are have a Gaussian/Normal distribution with the specified mean and standard deviation
template <class T>
T RandomNumberGenerator::GetGaussian(T mean, T std_dev)
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
   template T RandomNumberGenerator::GetInt(T min, T max);

#define MAKE_REAL_METHODS(T) \
   template T RandomNumberGenerator::GetReal(T min, T max); \
   template T RandomNumberGenerator::GetGaussian(T mean, T std_dev);

MAKE_INT_METHODS(int)
MAKE_INT_METHODS(unsigned int)
MAKE_INT_METHODS(long long)
MAKE_INT_METHODS(unsigned long long)

MAKE_REAL_METHODS(float)
MAKE_REAL_METHODS(double)
