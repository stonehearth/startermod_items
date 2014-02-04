local Point3 = _radiant.csg.Point3
local Fabricator = require 'components.fabricator.fabricator'

local TeardownStructure = class()
TeardownStructure.name = 'teardown structure'
TeardownStructure.does = 'stonehearth:teardown_structure'
TeardownStructure.args = {
   fabricator = Fabricator
}
TeardownStructure.version = 2
TeardownStructure.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(TeardownStructure)
         :execute('stonehearth:wait_for_teardown', { fabricator = ai.ARGS.fabricator })
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.fabricator:get_entity() })
         :execute('stonehearth:reserve_entity_destination', { entity = ai.ARGS.fabricator:get_entity(),
                                                              location = ai.PREV.point_of_interest })
         :execute('stonehearth:teardown_structure_adjacent', { fabricator = ai.ARGS.fabricator,
                                                               block = ai.PREV.location })
