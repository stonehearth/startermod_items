require 'radiant_tests.micro_world'
local dkjson = require 'dkjson'
local gm = require 'radiant.core.gm'
local MicroWorld = require 'radiant_tests.micro_world'

local CraftTest = class(MicroWorld)

function CraftTest:start()
   self:create_world()
   self:place_item_cluster('module://stonehearth/resources/oak_tree/oak_log', 2, 2)

   local recipe = 'wooden_practice_sword'
   local ingredients = dkjson.encode({ blade = 'oak-log', hilt = 'oak-log'})

   local carpenter = self:place_citizen(0, 10)
   om:add_component(carpenter, 'profession'):learn_recipe(recipe)
     
   self:at(10,  function()  ch:call('radiant.commands.craft', carpenter, recipe, ingredients) end)
end

gm:register_scenario('radiant.tests.craft_test', CraftTest)
