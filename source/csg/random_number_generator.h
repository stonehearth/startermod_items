#ifndef _RADIANT_CSG_RANDOM_NUMBER_GENERATOR_H
#define _RADIANT_CSG_RANDOM_NUMBER_GENERATOR_H

#include <random>
#include "namespace.h"

BEGIN_RADIANT_CSG_NAMESPACE

class RandomNumberGenerator
{
public:
   // Create a new RandomNumberGenerator seeded using the high resolution timer
   RandomNumberGenerator();

   // Create a new RandomNumberGenerator seeded using the specified seed
   RandomNumberGenerator(unsigned int seed);

   // Seed can be set at any time. All subsequent numbers will start using the new seed.
   void SetSeed(unsigned int seed);

   // Generates integers that are uniformly distributed over the interval [min, max] (min and max inclusive)
   template <class T>
   T GenerateUniformInt(T min, T max);

   // Generates real numbers that are uniformly distributed over the interval [min, max) (includes min, excludes max)
   template <class T>
   T GenerateUniformReal(T min, T max);

   // Generates real numbers that are have a Gaussian/Normal distribution with the specified mean and standard deviation
   template <class T>
   T GenerateGaussian(T mean, T std_dev);

private:
   std::default_random_engine generator_;
};

std::ostream& operator<<(std::ostream& out, const RandomNumberGenerator& source);

END_RADIANT_CSG_NAMESPACE

#endif _RADIANT_CSG_RANDOM_NUMBER_GENERATOR_H
