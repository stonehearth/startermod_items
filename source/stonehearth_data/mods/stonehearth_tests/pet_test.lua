local MicroWorld = require 'lib.micro_world'

local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local PetTest = class(MicroWorld)

function PetTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local trapper = self:place_citizen(7, 7, 'trapper')
   local faction = radiant.entities.get_faction(trapper)
   local rabbit = self:place_item('stonehearth:rabbit', -7, -7)

   -- tame the rabbit
   local equipment = rabbit:add_component('stonehearth:equipment')
   equipment:equip_item('stonehearth:pet_collar')
   
   --Make the rabbit from the same faction as the trapper
   radiant.entities.set_faction(rabbit, faction)

   local scheduler = stonehearth.tasks:create_scheduler('stonehearth:pet_scheduler')

   local task_group = scheduler:create_task_group('stonehearth:top')
                               :set_priority(stonehearth.constants.priorities.top.WORK)
                               :add_worker(rabbit)
                               :set_counter_name('pets')

   local task = task_group:create_task('stonehearth:follow_entity', { target = trapper })
                          :times(100)
                          :start()
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

   -- self:at(3000, function()
   --    rabbit:add_component('stonehearth:attributes'):set_attribute('sleepiness', 100)
   --    end)

return PetTest
