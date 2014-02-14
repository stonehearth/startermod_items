local Point3 = _radiant.csg.Point3
local MicroWorld = require 'lib.micro_world'
local PetTest = class(MicroWorld)
local rng = _radiant.csg.get_default_rng()

function PetTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local trapper = self:place_citizen(7, 7,'trapper')

   --local rabbit = self:place_citizen(0, 0,'trapper')
   local faction = radiant.entities.get_faction(trapper)
   local rabbit = self:place_item('stonehearth:rabbit', 0, 0)

   --TODO: move this functionality to a "make pet" function/action somewhere
   --TODO: add sleep/eat behaviors to the rabbit
   --TODO: in orchestrator, ever hour, have rabbit pick different people/
   --decide to slack off

   --Slap the pet collar on the rabbit
   local equipment = rabbit:add_component('stonehearth:equipment')
   equipment:equip_item('stonehearth:pet_collar')

   --Make the rabbit from the same faction as the trapper
   radiant.entities.set_faction(rabbit, faction)

   --TODO: create a pet orchestrator so pets can do multiple actions
   --Create a scheduler, have it do top (for now)
   local scheduler = stonehearth.tasks:create_scheduler()
                     :set_activity('stonehearth:top')
                     :join(rabbit)

   --Generate the target entity to follow (changing the target goes in the orchestrator)
   local target = self:_pick_target(rabbit)

   --Create the task with the trapper as the target
   scheduler:create_task('stonehearth:follow_entity', {target = target}):start()
end

--TODO: move to the orchestrator, when we have it, so we can alternate people
--we're obsessed with
function PetTest:_pick_target(entity)
   --if we don't have an idol, pick one based on our faction
   local faction = radiant.entities.get_faction(entity)
   local population_faction = stonehearth.population:get_faction(faction)
   local citizen_table = population_faction:get_citizens()
   local i = rng:get_int(1, #citizen_table)
   local target = citizen_table[i]

   entity:get_component('unit_info'):set_description("Fascinated by " .. target:get_component('unit_info'):get_display_name())
   return target
end

return PetTest