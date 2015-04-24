local Point3 = _radiant.csg.Point3
local Fabricator = require 'components.fabricator.fabricator'

local FabricateStructure = class()
FabricateStructure.name = 'fabricate structure'
FabricateStructure.does = 'stonehearth:fabricate_structure'
FabricateStructure.args = {
   fabricator = Fabricator
}
FabricateStructure.version = 2
FabricateStructure.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(FabricateStructure)
         :execute('stonehearth:find_path_to_entity', { destination = ai.ARGS.fabricator:get_entity() })
         :execute('stonehearth:reserve_entity_destination', { entity = ai.ARGS.fabricator:get_entity(),
                                                              location = ai.PREV.path:get_destination_point_of_interest() })
         :execute('stonehearth:pickup_item_made_of', {
               material = ai.ARGS.fabricator:get_material(ai.BACK(2).path:get_destination_point_of_interest())
            })
         :execute('stonehearth:goto_location', {
               reason = ai.ARGS.fabricator:get_description(),
               location = ai.BACK(3).path:get_finish_point(),
               stop_when_adjacent = false,
            })
         :execute('stonehearth:fabricate_structure_adjacent', { fabricator = ai.ARGS.fabricator,
                                                                block = ai.BACK(4).path:get_destination_point_of_interest() })
