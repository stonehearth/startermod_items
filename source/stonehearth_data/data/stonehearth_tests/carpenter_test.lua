local MicroWorld = require 'stonehearth_tests.lib.micro_world'
local CraftOrder = radiant.mods.require('/stonehearth_crafter/lib/craft_order.lua')

local CarpenterTest = class(MicroWorld)
--[[
   Instantiate a carpenter, a workbench, and a piece of wood.
   Turn out a wooden sword
]]

function CarpenterTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   --Create the carpenter, bench, and instantiate them to each other
   
   local carpenter = self:place_citizen(12, 12,'carpenter')
   local bench = self:place_item('/stonehearth_carpenter_class/entities/carpenter_workbench', -12, -12)
   
   local i = 1
   radiant.events.listen('radiant.events.slow_poll', function()
         i = i + 1
         bench:get_component('unit_info'):set_description(string.format('iteration %d', i))
      end)
   
   --TODO: can we do this via promote?
   local carpenter_component = carpenter:get_component('stonehearth_crafter:crafter')
   local workshop_component = bench:get_component('stonehearth_crafter:workshop')
   local faction = carpenter:get_component('unit_info'):get_faction()
   bench:add_component('unit_info'):set_faction(faction)
   workshop_component:set_crafter(carpenter)
   carpenter_component:set_workshop(workshop_component)
   -- end TODO

   -- put some items in the world
   --self:place_item_cluster('/stonehearth_trees/entities/oak_tree/oak_log', -10, 10, 3, 3)
   self:place_item_cluster('/stonehearth_items/cloth_bolt', -7, 10, 2, 2)

 -- Tests!

   ---[[
   --500ms seconds in, create an order for a shield (multiple types of ingredients)
   self:at(2000, function()
         --Programatically add items to the workbench's queue
         local condition = {}
         condition.amount = 1
         local order = CraftOrder(radiant.resources.load_json(
            '/stonehearth_carpenter_class/recipes/wooden_buckler_recipe.json'),
            true,  condition, workshop_component)
         local todo = workshop_component:ui_get_todo_list()
         todo:add_order(order)
      end)
   --]]

   ---[[
   --Create an order for a sword (multiple of single ingredient)
   self:at(3000, function()
         --Programatically add items to the workbench's queue
         local condition = {}
         condition.amount = 1
         local order = CraftOrder(radiant.resources.load_json(
            '/stonehearth_carpenter_class/recipes/wooden_sword_recipe.json'),
            true,  condition, workshop_component)
         local todo = workshop_component:ui_get_todo_list()
         todo:add_order(order)
      end)
   --]]

   ---[[
   --Create an order for 2 swords (multiple items in one order)
   self:at(5000, function()
         --Programatically add items to the workbench's queue
         local condition = {}
         condition.amount = 2
         local order = CraftOrder(radiant.resources.load_json(
            '/stonehearth_carpenter_class/recipes/wooden_sword_recipe.json'),
            true,  condition, workshop_component)
         local todo = workshop_component:ui_get_todo_list()
         todo:add_order(order)
      end)
   --]]
end

return CarpenterTest

