--[[
   Tell a worker to collect food and resources from a plant. 
]]

local event_service = stonehearth.events
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local HarvestPlantsAction = class()
HarvestPlantsAction.name = 'harvest plant adjacent'
HarvestPlantsAction.does = 'stonehearth:harvest_plant_adjacent'
HarvestPlantsAction.args = {
   plant = Entity       -- the plant to harvest
}
HarvestPlantsAction.version = 2
HarvestPlantsAction.priority = 1

function HarvestPlantsAction:run(ai, entity, args)
   local plant = args.plant
   radiant.entities.turn_to_face(entity, plant)

   --Fiddle with the bush and pop the basket
   local factory = plant:get_component('stonehearth:renewable_resource_node')
   if factory then
      ai:execute('stonehearth:run_effect', { effect = 'fiddle' })
      local front_point = entity:get_component('mob'):get_location_in_front()
      factory:spawn_resource(Point3(front_point.x, front_point.y, front_point.z))
   end   
end

return HarvestPlantsAction