local Point3 = _radiant.csg.Point3

local Wander = class()

Wander.name = 'wander'
Wander.does = 'stonehearth:idle:bored'
Wander.args = { }
Wander.version = 2
Wander.priority = 1

-- XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
-- XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
-- Debugging...DWT_1D
Wander.does = 'stonehearth:top'
Wander.priority = 10
-- XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
-- XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

function Wander:__init(ai, entity)
   self._ai = ai
   self._entity = entity
end

function Wander:start_background_processing(ai, entity)
   if not self._entity:get_component('stonehearth:leash') then
      self._entity:add_component('stonehearth:leash'):set_to_entity_location(self._entity)
   end

   local leash_component = entity:get_component('stonehearth:leash')
   local leash_location = leash_component:get_location()
   local entity_location = radiant.entities.get_location_aligned(entity)

   local radius = 3
   local dx = math.random(-3, 3)
   local dz = math.random(-3, 3)

   if ((entity_location.x + dx > leash_location.x + radius) or 
       (entity_location.x + dx < leash_location.x - radius)) then

       dx = dx * -1;
   end

   if ((entity_location.z + dz > leash_location.z + radius) or 
       (entity_location.x + dz < leash_location.z - radius)) then

       dz = dz * -1;
   end   

   local destination = Point3(leash_location)
   destination.x = destination.x + dx
   destination.z = destination.z + dz
   
   ai:complete_background_processing(destination)
end

local ai = stonehearth.ai
return ai:create_compound_action(Wander)
         :execute('stonehearth:goto_location', ai.PREV[1], 'run')
