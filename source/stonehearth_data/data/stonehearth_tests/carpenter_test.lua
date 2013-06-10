local MicroWorld = require 'stonehearth_tests.lib.micro_world'
local CraftOrder = radiant.mods.require('mod://stonehearth_crafter/lib/craft_order.lua')

local CarpenterTest = class(MicroWorld)
--[[
   Instantiate a carpenter, a workbench, and a piece of wood. 
   Turn out a wooden sword
]]

function CarpenterTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   --Create the carpenter, bench, and instantiate them to each other
   --TODO: can we do this via promote?
   local carpenter = self:place_citizen(12, 12,'carpenter')
   local carpenter_component = carpenter:get_component('mod://stonehearth_crafter/components/crafter.lua')
   local bench = self:place_item('mod://stonehearth_carpenter_class/entities/carpenter_workbench', -12, -12)
   local workshop_component = bench:get_component('mod://stonehearth_crafter/components/workshop.lua')
   workshop_component:add_crafter(carpenter_component)
   carpenter_component:set_workshop(workshop_component)

-- Tests!

 ---[[
   --500ms seconds in, create an order for a shield (multiple types of ingredients)
   self:at(500, function()
         --Programatically add items to the workbench's queue
         local condition = {}
         condition.amount = 1
         local order = CraftOrder(radiant.resources.load_json(
            'mod://stonehearth_carpenter_class/recipes/wooden_buckler_recipe.txt'),
            true,  condition, workshop_component)
         local todo = workshop_component:get_todo_list()
         todo:add_order(order)
      end)
--]]

---[[
   --Create an order for a sword (multiple of single ingredient)
   self:at(1000, function()
         --Programatically add items to the workbench's queue
         local condition = {}
         condition.amount = 1
         local order = CraftOrder(radiant.resources.load_json(
            'mod://stonehearth_carpenter_class/recipes/wooden_sword_recipe.txt'),
            true,  condition, workshop_component)
         local todo = workshop_component:get_todo_list()
         todo:add_order(order)
      end)
--]]

---[[
   --Create an order for 2 swords (multiple items in one order)
   self:at(10000, function()
         --Programatically add items to the workbench's queue
         local condition = {}
         condition.amount = 2
         local order = CraftOrder(radiant.resources.load_json(
            'mod://stonehearth_carpenter_class/recipes/wooden_sword_recipe.txt'),
            true,  condition, workshop_component)
         local todo = workshop_component:get_todo_list()
         todo:add_order(order)
      end)
--]]

end

return CarpenterTest

