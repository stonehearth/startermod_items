#include "pch.h"
#include "interfaces.h"
#include "simulation.h"

using ::radiant::simulation::Simulation;
using ::radiant::simulation::SimulationInterface;

SimulationInterface *::radiant::simulation::CreateSimulation()
{
   return new Simulation();
}

SimulationInterface::~SimulationInterface()
{
}
