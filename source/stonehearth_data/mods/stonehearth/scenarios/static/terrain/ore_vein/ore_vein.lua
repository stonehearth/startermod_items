local constants = require 'constants'
local Point3 = _radiant.csg.Point3

local OreVein = class()

function OreVein:__init()
end

function OreVein:initialize(properties, context, services)
   local location = self:_align_location(context.location)
   local ore_network = stonehearth.mining:create_ore_network(location, properties.kind, properties.data)
   local terrain_shape = radiant.terrain.intersect_region(ore_network)
   local intersected_ore_network = ore_network:intersect_region(terrain_shape)
   intersected_ore_network:optimize_by_merge()
   radiant.terrain.add_region(intersected_ore_network)
end

function OreVein:_align_location(location)
   local y_cell_size = constants.mining.Y_CELL_SIZE
   local aligned_location = Point3(location)

   -- make sure our origin is at the bottom of a slice
   aligned_location.y = math.floor(aligned_location.y / y_cell_size) * y_cell_size
   return aligned_location
end

return OreVein
