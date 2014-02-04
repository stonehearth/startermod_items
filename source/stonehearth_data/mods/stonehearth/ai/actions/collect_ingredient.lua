local IngredientList = require 'components.workshop.ingredient_list'
local Entity = _radiant.om.Entity

local CollectIngredient = class()
CollectIngredient.name = 'collect ingredient'
CollectIngredient.does = 'stonehearth:collect_ingredient'
CollectIngredient.args = {
   material = 'string',                   -- what to get
   workshop = Entity,                     -- where to drop it off
   ingredient_list = IngredientList,      -- the tracking list
}
CollectIngredient.version = 2
CollectIngredient.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(CollectIngredient)
               :execute('stonehearth:pickup_item_made_of', { material = ai.ARGS.material })
               :execute('stonehearth:drop_carrying_into_entity', { entity = ai.ARGS.workshop })
               :execute('stonehearth:call_method', {
                  obj = ai.ARGS.ingredient_list,
                  method = 'check_off_ingredient',
                  args = { ai.BACK(2).item }
               })
