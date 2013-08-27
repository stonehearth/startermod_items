require 'unclasslib'
local gm  = require 'radiant.core.gm'
local log = require 'radiant.core.log'
local sh = require 'radiant.core.sh'
local om = require 'radiant.core.om'

local names = {
   'Mer Burlyhands',
   'Illowyn Farstrider',
   'Samson Moonstoke',
   'Godric Greenskin',
   'Olaf Oakshield',
}

local Embark = class()

Embark['radiant.md.create'] = function(self, options)
   log:info('inside embark... (%s)', tostring(options.box))
   
   local center = options.box:center()
   for i = 1, 1 do
      local pt = center
      pt.x = pt.x + math.random(-8, 8)
      pt.y = 1
      pt.z = pt.z + math.random(-8, 8)
      
      local citizen = sh:create_citizen(Point3(pt))
    
      om:set_display_name(citizen, names[math.random(1, #names)])     
   end
   local saw = om:create_entity('carpenter-saw')
   om:place_on_terrain(saw, Point3(center))
end

gm:register_scenario('stonehearth.embark', Embark)

