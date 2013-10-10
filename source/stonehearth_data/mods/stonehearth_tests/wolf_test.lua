local Point3 = _radiant.csg.Point3
local MicroWorld = require 'lib.micro_world'
local WolfTest = class(MicroWorld)

function WolfTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()

   self:place_tree(-11, -11)
   --self:place_item('stonehearth:rabbit', -12, 12)
   self:place_item('stonehearth:wolf', -9, -9)

   local civ = self:place_citizen(12, 12)
   --self:place_tree(12, 16)
   --radiant.effects.run_effect(civ, '/stonehearth/data/effects/hit_sparks/blood_effect.json')

   --local bench = self:place_item('stonehearth:carpenter_workbench', -6, 6)
   
end

return WolfTest

