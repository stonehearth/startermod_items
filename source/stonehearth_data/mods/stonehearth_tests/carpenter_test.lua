local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3

local CarpenterTest = class(MicroWorld)
--[[
   Instantiate a carpenter, a workbench, and a piece of wood.
   Turn out a wooden sword
]]

function CarpenterTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   --Add a worker
   local worker = self:place_citizen(-5, -5)

   --Create the carpenter. You will have to create the bench as part of the test
   local carpenter = self:place_citizen(-12, 7, 'carpenter')
   local player_id = radiant.entities.get_player_id(carpenter)

   self:place_item('stonehearth:arch_backed_chair', 0, 0)
   self:place_item('stonehearth:comfy_bed', 1, 0)
   self:place_item('stonehearth:dining_table', 2, 0)
   self:place_item('stonehearth:picket_fence', 0, 1)
   self:place_item('stonehearth:picket_fence_gate', 1, 1)
   self:place_item('stonehearth:simple_wooden_chair', 2, 1)
   self:place_item('stonehearth:table_for_one', 3, 1)
   self:place_item('stonehearth:wooden_door', 4, 1)
   self:place_item('stonehearth:wooden_window_frame', 0, 2)
   self:place_item('stonehearth:picket_fence', 0, 3)
   self:place_item('stonehearth:picket_fence', 1, 3)
   self:place_item('stonehearth:picket_fence', 2, 3)
   self:place_item('stonehearth:picket_fence', 3, 3)
   self:place_item('stonehearth:picket_fence', 4, 3)
   self:place_item('stonehearth:picket_fence', 5, 3)
   self:place_item('stonehearth:picket_fence', 6, 3)
   self:place_item('stonehearth:firepit', 7, 3, player_id)
   self:place_item('stonehearth:firepit', 9, 3, player_id)

   self:place_item('stonehearth:berry_basket', 10, 10)
   self:place_item('stonehearth:corn_basket', 11, 11)

   -- put some items in the world
   self:place_item_cluster('stonehearth:oak_log', -10, 0, 10, 10)
   self:place_item_cluster('stonehearth:cloth_bolt', 10, 3, 3, 3)

 -- Tests!

   --[[
   --500ms seconds in, create an order for a shield (multiple types of ingredients)
   self:at(2000, function()
         --Programatically add items to the workbench's queue
         local condition = {}
         condition.amount = 1
         local order = CraftOrder(radiant.resources.load_json(
            '/stonehearth/professions/carpenter/recipes/wooden_buckler_recipe.json'),
            true,  condition, workshop_component)
         local todo = workshop_component:ui_get_todo_list()
         todo:add_order(order)
      end)
   --]]

   --[[
   --Create an order for a sword (multiple of single ingredient)
   self:at(3000, function()
         --Programatically add items to the workbench's queue
         local condition = {}
         condition.amount = 1
         local order = CraftOrder(radiant.resources.load_json(
            '/stonehearth/professions/carpenter/recipes/wooden_sword_recipe.json'),
            true,  condition, workshop_component)
         local todo = workshop_component:ui_get_todo_list()
         todo:add_order(order)
      end)
   --]]

   --[[
   --Create an order for 2 swords (multiple items in one order)
   self:at(5000, function()
         --Programatically add items to the workbench's queue
         local condition = {}
         condition.amount = 2
         local order = CraftOrder(radiant.resources.load_json(
            '/stonehearth/professions/carpenter/recipes/wooden_sword_recipe.json'),
            true,  condition, workshop_component)
         local todo = workshop_component:ui_get_todo_list()
         todo:add_order(order)
      end)
   --]]
end

return CarpenterTest

