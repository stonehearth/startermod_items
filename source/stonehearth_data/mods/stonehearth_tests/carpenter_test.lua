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

   self:place_item('stonehearth:arch_backed_chair_proxy', 0, 0)
   self:place_item('stonehearth:comfy_bed_proxy', 1, 0)
   self:place_item('stonehearth:dining_table_proxy', 2, 0)
   self:place_item('stonehearth:picket_fence_proxy', 0, 1)
   self:place_item('stonehearth:picket_fence_gate_proxy', 1, 1)
   self:place_item('stonehearth:simple_wooden_chair_proxy', 2, 1)
   self:place_item('stonehearth:table_for_one_proxy', 3, 1)
   self:place_item('stonehearth:wooden_door_proxy', 4, 1)
   self:place_item('stonehearth:wooden_window_frame_proxy', 0, 2)
   self:place_item('stonehearth:picket_fence_proxy', 0, 3)
   self:place_item('stonehearth:picket_fence_proxy', 1, 3)
   self:place_item('stonehearth:picket_fence_proxy', 2, 3)
   self:place_item('stonehearth:picket_fence_proxy', 3, 3)
   self:place_item('stonehearth:picket_fence_proxy', 4, 3)
   self:place_item('stonehearth:picket_fence_proxy', 5, 3)
   self:place_item('stonehearth:picket_fence_proxy', 6, 3)
   self:place_item('stonehearth:fire_pit_proxy', 7, 3)
   self:place_item('stonehearth:fire_pit', 9, 3)

   --self:place_item('stonehearth:not_much_of_a_bed_proxy', 4, 4)

   --Create the carpenter, bench, and instantiate them to each other

   local bench = self:place_item('stonehearth:carpenter_workbench', -12, 12)
   local workshop_component = bench:get_component('stonehearth:workshop')

   local carpenter = self:place_citizen(-12, 7,'carpenter', workshop_component)

   local faction = carpenter:get_component('unit_info'):get_faction()
   bench:add_component('unit_info'):set_faction(faction)

   local outbox = workshop_component:init_outbox()
   outbox:add_component('unit_info'):set_faction(faction)

   --initialize the outbox
   --user places both workshop and outbox
   --TODO: make a private stockpile

   --[[
   local outbox_entity = radiant.entities.create_entity('stonehearth:stockpile')
   local outbox_location = Point3(-8, 1, -12)
   radiant.terrain.place_entity(outbox_entity, outbox_location)
   local outbox_component = outbox_entity:get_component('stonehearth:stockpile')
   outbox_component:set_size({3, 3})
   outbox_entity:get_component('unit_info'):set_faction(faction)
   workshop_component:set_outbox(outbox_entity)
   ]]
   -- end TODO

   -- put some items in the world
   self:place_item_cluster('stonehearth:oak_log', -10, 0, 3, 3)
   self:place_item_cluster('stonehearth:cloth_bolt', -7, 0, 2, 2)

   --TODO: figure out iconic objects
   --self:place_item_cluster('stonehearth:comfy_bed', 0, 0, 2, 2)

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

