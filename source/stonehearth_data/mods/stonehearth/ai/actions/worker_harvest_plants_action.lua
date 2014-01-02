--[[
   Tell a worker to collect food and resources from a plant. 
]]

local event_service = require 'services.event.event_service'
local Point3 = _radiant.csg.Point3

local HarvestPlantsAction = class()
local log = radiant.log.create_logger('actions.harvest')

HarvestPlantsAction.name = 'harvest plants'
HarvestPlantsAction.does = 'stonehearth:harvest_plant'
HarvestPlantsAction.priority = 9

function HarvestPlantsAction:__init(ai, entity)
   self._ai = ai
   self._entity = entity

end

function HarvestPlantsAction:run(ai, entity, path)
   local plant = path:get_destination()

   --is the plant the berries or the bush? 
   local is_harvestable_item =  radiant.entities.get_entity_data(plant, 'stonehearth:havestable_item')
   if is_harvestable_item then
      --we actually want its parent, the plant that contains the harvestable item
      plant = plant:get_component('mob'):get_parent()
   end

   if not plant then
      ai:abort('plant does not exist anymore in stonehearth:harvest_plant')
   end

   local worker_name = radiant.entities.get_display_name(entity)
   local plant_name = radiant.entities.get_display_name(plant)

   -- Log successful grab
   -- TODO: localize, and put in log!
   --event_service:add_entry(worker_name .. ' is collecting stuff from a ' .. plant_name)

   ai:execute('stonehearth:follow_path', path)
   radiant.entities.turn_to_face(entity, plant)

   --Fiddle with the bush and pop the basket
   local factory = plant:get_component('stonehearth:renewable_resource_node')
   if factory then
      ai:execute('stonehearth:run_effect','fiddle')
      local front_point = self._entity:get_component('mob'):get_location_in_front()
      factory:spawn_resource(Point3(front_point.x, front_point.y, front_point.z))
   end   

end

return HarvestPlantsAction