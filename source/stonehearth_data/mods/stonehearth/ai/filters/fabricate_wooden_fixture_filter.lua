local Entity = _radiant.om.Entity
local FixtureFabricator = require 'components.fixture_fabricator.fixture_fabricator_component'

local FabricateWoodenFixture = class()
FabricateWoodenFixture.type = 'filter'
FabricateWoodenFixture.name = 'fabricate wooden fixture'
FabricateWoodenFixture.does = 'stonehearth:fabricate_fixture'
FabricateWoodenFixture.args = {
   fabricator = Entity,
   fixture_uri = 'string',
}
FabricateWoodenFixture.version = 2
FabricateWoodenFixture.priority = 1

-- Only do this if the fixture is wood
function FabricateWoodenFixture:should_start_thinking(ai, entity, args)
   local fixture_data = radiant.resources.load_json(args.fixture_uri)
   if fixture_data then
      local material_data = fixture_data.components['stonehearth:material']
      if material_data then
         local material = material_data.tags
         if string.find(material, 'wood') then
            return true
         end
      end         
   end
   return false
end

return FabricateWoodenFixture
