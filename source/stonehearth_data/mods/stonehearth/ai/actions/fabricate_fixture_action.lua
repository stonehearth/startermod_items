local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local FixtureFabricator = require 'components.fixture_fabricator.fixture_fabricator_component'

local FabricateFixture = class()
FabricateFixture.name = 'fabricate fixture'
FabricateFixture.does = 'stonehearth:fabricate_fixture'
FabricateFixture.args = {
   fabricator = Entity,
   fixture_uri = 'string',
}
FabricateFixture.version = 2
FabricateFixture.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(FabricateFixture)
         :execute('stonehearth:pickup_item_with_uri', { uri = ai.ARGS.fixture_uri })
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.fabricator })
         :execute('stonehearth:fabricate_fixture_adjacent', { fabricator = ai.ARGS.fabricator, item = ai.BACK(2).item })
