#include "pch.h"
#include "interfaces.h"
#include "simulation.h"

using ::radiant::simulation::Simulation;
using ::radiant::simulation::SimulationInterface;

SimulationInterface *::radiant::simulation::CreateSimulation(lua_State* L)
{
   return new Simulation(L);
}

SimulationInterface::~SimulationInterface()
{
}
