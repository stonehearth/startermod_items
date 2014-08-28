local Point3 = _radiant.csg.Point3
local MicroWorld = require 'lib.micro_world'
local TrapperTest = class(MicroWorld)

function TrapperTest:__init()
   self[MicroWorld]:__init(128)
   self:create_world()

   self:place_item('stonehearth:large_oak_tree', -25, -25)
   self:place_item('stonehearth:medium_oak_tree', -5, -25)
   self:place_item('stonehearth:small_oak_tree',  15, -25)

   self:place_item('stonehearth:large_juniper_tree', -25, -5)
   self:place_item('stonehearth:medium_juniper_tree', -5, -5)
   self:place_item('stonehearth:small_juniper_tree',  15, -5)

   self:place_item('stonehearth:large_boulder',  -25, 5)
   self:place_item('stonehearth:medium_boulder', -15, 5)
   self:place_item('stonehearth:small_boulder',   -5, 5)

   self:place_item('stonehearth:small_boulder',    5, 5)
       :add_component('mob'):turn_to(90)

   self:place_item('stonehearth:small_boulder',    15, 5)
       :add_component('mob'):turn_to(90)

   self:place_item('stonehearth:berry_bush', -25, 15)
   self:place_item('stonehearth:berry_bush', -15, 15)
   self:place_item('stonehearth:silkweed',  -5, 15)

   self:place_citizen(10, 10, 'stonehearth:professions:trapper')
   --self:place_citizen(8, 8, 'stonehearth:professions:trapper')
   --self:place_citizen(8, 8, 'stonehearth:professions:footman')
   -- self:place_citizen(12, 12)
   -- self:place_citizen(14, 14)

   self:place_item('stonehearth:red_fox', 15, 15, 'game_master')
   -- self:place_critter('stonehearth:rabbit', 14, 14, 'game_master')
   -- self:place_critter('stonehearth:racoon', 13, 13, 'game_master')
   -- self:place_critter('stonehearth:squirrel', 12, 12, 'game_master')
end

return TrapperTest
