local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local FixtureFabricator = require 'components.fixture_fabricator.fixture_fabricator_component'

local FabricateWoodenFixture = class()
FabricateWoodenFixture.name = 'fabricate wooden fixture'
FabricateWoodenFixture.does = 'stonehearth:fabricate_fixture'
FabricateWoodenFixture.args = {
   fabricator = Entity,
   fixture_uri = 'string',
}
FabricateWoodenFixture.version = 2
FabricateWoodenFixture.priority = 1

-- Only do this if the fixture is wood
function FabricateWoodenFixture:start_thinking(ai, entity, args)
   local fixture_data = radiant.resources.load_json(args.fixture_uri)
   if fixture_data then
      local material_data = fixture_data.components['stonehearth:material']
      if material_data then
         local material = material_data.tags
         if string.find(material, 'wood') then
            ai:set_think_output({})
         end
      end         
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(FabricateWoodenFixture)
         :execute('stonehearth:pickup_item_with_uri', { uri = ai.ARGS.fixture_uri })
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.fabricator })
         :execute('stonehearth:fabricate_fixture_adjacent', { fabricator = ai.ARGS.fabricator, item = ai.BACK(2).item })
