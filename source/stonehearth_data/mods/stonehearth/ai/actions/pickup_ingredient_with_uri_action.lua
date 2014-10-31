local IngredientList = require 'components.workshop.ingredient_list'
local Entity = _radiant.om.Entity

local PickupIngredientWithUri = class()
PickupIngredientWithUri.name = 'pickup entity type ingredient'
PickupIngredientWithUri.does = 'stonehearth:pickup_ingredient'
PickupIngredientWithUri.args = {
   ingredient = 'table',                  -- what to get
}

PickupIngredientWithUri.version = 2
PickupIngredientWithUri.priority = 1

function PickupIngredientWithUri:start_thinking(ai, entity, args)
   if args.ingredient.uri ~= nil then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupIngredientWithUri)
               :execute('stonehearth:pickup_item_with_uri', { uri = ai.ARGS.ingredient.uri })
