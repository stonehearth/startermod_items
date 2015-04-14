local Point3 = _radiant.csg.Point3
local Fabricator = require 'components.fabricator.fabricator'

local FabricateWoodenStructure = class()
FabricateWoodenStructure.type = 'filter'
FabricateWoodenStructure.name = 'fabricate wooden structure'
FabricateWoodenStructure.does = 'stonehearth:fabricate_structure_adjacent'
FabricateWoodenStructure.args = {
   fabricator = Fabricator,
   block = Point3,
}
FabricateWoodenStructure.version = 2
FabricateWoodenStructure.priority = 1

-- Only do this task if we're building something made of wood. 
function FabricateWoodenStructure:should_start_thinking(ai, entity, args)
   local material = args.fabricator:get_material(args.block)
   if string.find(material, 'wood') then
      return true
   end
   return false
end

return FabricateWoodenStructure
