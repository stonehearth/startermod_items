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
// 68% of values fall within 1 standard deviation of the mean
// 95% of values fall within 2 standard deviations of the mean
template <class T>
T RandomNumberGenerator::GetGaussian(T mean, T std_dev)
{
   static_assert(std::is_floating_point<T>::value, "<T> must be a floating point type");

   std::normal_distribution<T> distribution(mean, std_dev);
   return distribution(generator_);
}

static std::string const SerializationHeader = "RandomNumberGenerator(";
static std::string const SerializationFooter = ")";

static bool MatchString(std::istream& in, std::string const& str)
{
   std::string data;
   data.resize(str.length());
   in.read(&data[0], str.length());
   return data == str;
}

std::ostream& csg::operator<<(std::ostream& out, RandomNumberGenerator const& rng)
{
   out << SerializationHeader << rng.generator_ << SerializationFooter;
   return out;
}

std::istream& csg::operator>>(std::istream& in, RandomNumberGenerator& rng)
{
   std::string header, footer;

   // read header
   bool success = MatchString(in, SerializationHeader);
   if (!success) {
      throw core::Exception("Bad stream header when attempting to deserialize RandomNumberGenerator");
   }

   // deserialize the internal state of the generator
   // Note: the generator_ implementation does not consume the final space!
   in >> rng.generator_;

   // Hack - read the orphaned space
   if (in.peek() == ' ') {
      in.get();
   }

   // read footer
   success = MatchString(in, SerializationFooter);
   if (!success) {
      throw core::Exception("Bad stream footer when attempting to deserialize RandomNumberGenerator");
   }

   return in;
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
