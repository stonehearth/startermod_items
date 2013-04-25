local dkjson = require 'dkjson'
local ch = require 'radiant.core.ch'

ch:register_cmd("radiant.commands.craft", function(crafter, recipe_name, ingredients_str)
   check:is_entity(crafter)
   local recipe = native:lookup_resource(recipe_name)
   if not recipe then
      return { error = 'unknown recipe ' .. recipe_name }
   end
   local ingredients = dkjson.decode(ingredients_str)
   
   md:send_msg(crafter, 'radiant.add_automation_task', 'radiant.xxx.craft', recipe_name, ingredients_str)
   return { }
end)

