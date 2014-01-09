local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_random_number_generator()

local ChoosePointAroundLeash = class()

ChoosePointAroundLeash.name = 'choose point around leash'
ChoosePointAroundLeash.does = 'stonehearth:choose_point_around_leash'
ChoosePointAroundLeash.args = {
  radius  = 'number'     -- how far?
}
ChoosePointAroundLeash.think_output = {
  location = Point3     -- where to wander off to
}
ChoosePointAroundLeash.version = 2
ChoosePointAroundLeash.priority = 2

function ChoosePointAroundLeash:__init(ai, entity)
   self._ai = ai
   self._entity = entity
end

function ChoosePointAroundLeash:start_thinking(ai, entity, args)
   if not self._entity:get_component('stonehearth:leash') then
      self._entity:add_component('stonehearth:leash'):set_to_entity_location(self._entity)
   end

   local leash_component = entity:get_component('stonehearth:leash')
   local leash_location = leash_component:get_location()
   local entity_location = radiant.entities.get_location_aligned(entity)

   local radius = args.radius
   local dx = rng:get_int(-radius, radius)
   local dz = rng:get_int(-radius, radius)

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
   
   ai:set_think_output({ location = destination })
end

return ChoosePointAroundLeash
