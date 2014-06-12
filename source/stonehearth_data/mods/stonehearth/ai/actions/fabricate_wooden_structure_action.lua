local Point3 = _radiant.csg.Point3
local Fabricator = require 'components.fabricator.fabricator'

local FabricateWoodenStructure = class()
FabricateWoodenStructure.name = 'fabricate wooden structure'
FabricateWoodenStructure.does = 'stonehearth:fabricate_structure'
FabricateWoodenStructure.args = {
   fabricator = Fabricator
}
FabricateWoodenStructure.version = 2
FabricateWoodenStructure.priority = 1

-- Only do this task if we're building something made of wood. 
function FabricateWoodenStructure:start_thinking(ai, entity, args)
   local material = args.fabricator:get_material()
   if string.find(material, 'wood') then
      ai:set_think_output({})
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(FabricateWoodenStructure)
         :execute('stonehearth:pickup_item_made_of', { material = ai.ARGS.fabricator:get_material() })
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.fabricator:get_entity() })
         :execute('stonehearth:reserve_entity_destination', { entity = ai.ARGS.fabricator:get_entity(),
                                                              location = ai.PREV.point_of_interest })
         :execute('stonehearth:fabricate_structure_adjacent', { fabricator = ai.ARGS.fabricator,
                                                                block = ai.PREV.location })
