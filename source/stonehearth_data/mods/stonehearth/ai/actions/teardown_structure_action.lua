local Point3 = _radiant.csg.Point3
local Fabricator = require 'components.fabricator.fabricator_component'

local TeardownStructure = class()
TeardownStructure.name = 'teardown structure'
TeardownStructure.does = 'stonehearth:teardown_structure'
TeardownStructure.args = {
   material = 'string',
   fabricator = Fabricator,
}
TeardownStructure.version = 2
TeardownStructure.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(TeardownStructure)
         :execute('stonehearth:drop_carrying_if_stacks_full', {})
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.fabricator:get_material_proxy(ai.ARGS.material) })
         :execute('stonehearth:reserve_entity_destination', { entity = ai.ARGS.fabricator:get_material_proxy(ai.ARGS.material),
                                                              location = ai.PREV.point_of_interest })
         :execute('stonehearth:teardown_structure_adjacent', { fabricator = ai.ARGS.fabricator,
                                                               block = ai.PREV.location })
         :execute('stonehearth:drop_carrying_if_stacks_full', {})
