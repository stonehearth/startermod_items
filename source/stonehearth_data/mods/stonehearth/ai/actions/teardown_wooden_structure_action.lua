local Point3 = _radiant.csg.Point3
local Fabricator = require 'components.fabricator.fabricator'

local TeardownWoodenStructure = class()
TeardownWoodenStructure.name = 'teardown wooden structure'
TeardownWoodenStructure.does = 'stonehearth:teardown_structure'
TeardownWoodenStructure.args = {
   fabricator = Fabricator
}
TeardownWoodenStructure.version = 2
TeardownWoodenStructure.priority = 1

-- Only do this task if we're building something made of wood. 
function TeardownWoodenStructure:start_thinking(ai, entity, args)
   local material = args.fabricator:get_material()
   if string.find(material, 'wood') then
      ai:set_think_output({})
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(TeardownWoodenStructure)
         :execute('stonehearth:drop_carrying_if_stacks_full', {})
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.fabricator:get_entity() })
         :execute('stonehearth:reserve_entity_destination', { entity = ai.ARGS.fabricator:get_entity(),
                                                              location = ai.PREV.point_of_interest })
         :execute('stonehearth:teardown_structure_adjacent', { fabricator = ai.ARGS.fabricator,
                                                               block = ai.PREV.location })
         :execute('stonehearth:drop_carrying_if_stacks_full', {})
