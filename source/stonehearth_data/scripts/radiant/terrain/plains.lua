local Plains = class()

local tg = require 'radiant.core.tg'
local om = require 'radiant.core.om'

function Plains:create_layer(region)
   local grid = om:get_terrain_grid()
      
   grid:set_palette_entry(1, 128, 128, 128)
   grid:grow_bounds(region)
   grid:set_block(region, 1)
end

tg:register_terrain(Plains, {
   name = 'plains'   
})
