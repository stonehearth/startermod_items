#include "pch.h"
#include "action.h"
#include "simulation/simulation.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

Action::Action()
{
}

Action::~Action()
{
}

void Action::OnEvent(const Event& e, bool &consumed)
{
}

void Action::Start()
{
   finished_ = false;
}

void Action::Stop()
{
   finished_ = true;
}

void Action::Finish()
{
   finished_ = true;
}

bool Action::Run()
{
   ASSERT(!finished_);

   if (!finished_) {
      Update();
   }
   return finished_;
}
