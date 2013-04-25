#ifndef _RADIANT_SIMULATION_ACTION_H
#define _RADIANT_SIMULATION_ACTION_H

#include "om/om.h"
#include "physics/namespace.h"
#include "simulation/namespace.h"

struct lua_State;

BEGIN_RADIANT_SIMULATION_NAMESPACE

class Event;

class Action {
public:
   Action();
   virtual ~Action();

public:
   bool Run();

   bool IsFinished() const { return finished_; }
   virtual void Create(lua_State* L) = 0;
   virtual void OnEvent(const Event& e, bool &consumed);
   virtual void Update() = 0;
   virtual void Start();
   virtual void Stop();

protected:
   virtual void Finish();

protected:
   bool                 finished_;
};


END_RADIANT_SIMULATION_NAMESPACE

#endif // _RADIANT_SIMULATION_ACTION_H

