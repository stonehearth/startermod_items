local Fabricator = require 'components.fabricator.fabricator'

local Point3 = _radiant.csg.Point3

local TeardownWoodenStructure = class()
TeardownWoodenStructure.type = 'filter'
TeardownWoodenStructure.name = 'teardown wooden structure'
TeardownWoodenStructure.does = 'stonehearth:teardown_structure_adjacent'
TeardownWoodenStructure.args = {
   fabricator = Fabricator,
   block = Point3,
}
TeardownWoodenStructure.version = 2
TeardownWoodenStructure.priority = 1

-- Only do this task if we're building something made of wood. 
function TeardownWoodenStructure:should_start_thinking(ai, entity, args)
   local material = args.fabricator:get_material(args.block)
   if string.find(material, 'wood') then
      return true
   end
   return false
end

return TeardownWoodenStructure
