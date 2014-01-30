#include "radiant.h"
#include "counter.h"

using namespace radiant;
using namespace radiant::perfmon;

Counter::Counter(char const* name) :
   name_(name),
   value_(0)
{
}

Counter::~Counter()
{
}

const char* Counter::GetName() const
{
   return name_;
}

CounterValueType Counter::GetValue() const
{
   return value_;
}

void Counter::SetValue(CounterValueType value)
{
   value_ = value;
}

void Counter::Increment(CounterValueType amount)
{
   value_ += amount;
}

void Counter::Decrement(CounterValueType amount)
{
   value_ -= amount;
}
