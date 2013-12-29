#ifndef _RADIANT_CSG_RANDOM_NUMBER_GENERATOR_H
#define _RADIANT_CSG_RANDOM_NUMBER_GENERATOR_H

#include <random>
#include <mutex>
#include <boost/thread/thread.hpp>
#include "namespace.h"
#include "radiant_macros.h"

BEGIN_RADIANT_CSG_NAMESPACE

class RandomNumberGenerator
{
public:
   // Gets the default instance for this thread
   // If you want to set the seed, you need to set it on the instance after the thread is created
   static RandomNumberGenerator& DefaultInstance();

   // Create a new RandomNumberGenerator seeded using the high resolution timer
   RandomNumberGenerator();

   // Create a new RandomNumberGenerator seeded using the specified seed
   RandomNumberGenerator(unsigned int seed);

   // Seed can be set at any time. All subsequent numbers will start using the new seed.
   void SetSeed(unsigned int seed);

   // Generates integers that are uniformly distributed over the interval [min, max] (min and max inclusive)
   template <class T>
   T GetInt(T min, T max);

   // Generates real numbers that are uniformly distributed over the interval [min, max) (includes min, excludes max)
   template <class T>
   T GetReal(T min, T max);

   // Generates real numbers that are have a Gaussian/Normal distribution with the specified mean and standard deviation
   template <class T>
   T GetGaussian(T mean, T std_dev);

private:
   NO_COPY_CONSTRUCTOR(RandomNumberGenerator);

   static std::map<boost::thread::id, std::unique_ptr<RandomNumberGenerator>> instances_;
   static std::recursive_mutex mutex_;

   std::default_random_engine generator_;
};

std::ostream& operator<<(std::ostream& out, RandomNumberGenerator const& source);

END_RADIANT_CSG_NAMESPACE

#endif _RADIANT_CSG_RANDOM_NUMBER_GENERATOR_H
