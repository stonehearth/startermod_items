local constants = require 'constants'

local OreVein = class()

function OreVein:__init()
end

function OreVein:initialize(properties, services)
   local y_cell_size = constants.mining.Y_CELL_SIZE
   local location = properties.location
   local kind = properties.kind

   -- make sure our origin is at the bottom of a slice
   location.y = math.floor(location.y / y_cell_size) * y_cell_size

   local ore_network = stonehearth.mining:create_ore_network(location, kind)
   local terrain_shape = radiant.terrain.intersect_region(ore_network)
   local intersected_ore_network = ore_network:intersect_region(terrain_shape)
   intersected_ore_network:optimize_by_merge()
   radiant.terrain.add_region(intersected_ore_network)
end

return OreVein
