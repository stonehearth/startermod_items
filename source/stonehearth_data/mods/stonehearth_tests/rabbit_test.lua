local Point3 = _radiant.csg.Point3
local MicroWorld = require 'lib.micro_world'
local RabbitTest = class(MicroWorld)

function RabbitTest:__init()
   self[MicroWorld]:__init()
   self:create_world()
   self:place_item('stonehearth:small_boulder', 0, 0)
   if true then return false end

   local tree = self:place_tree(-12, -12)
   local tree = self:place_tree(-12, 12)
   
   self:place_citizen(6, 6, 'trapper') 
   --self:place_item('stonehearth:sheep', -3, -6)
   self:place_item('stonehearth:racoon', 3, 3)
   self:place_item('stonehearth:red_fox', 3, -3)
   self:place_item('stonehearth:squirrel', -3, 3)


   --local bench = self:place_item('stonehearth:carpenter:workbench', -6, 6)
   
end

return RabbitTest

