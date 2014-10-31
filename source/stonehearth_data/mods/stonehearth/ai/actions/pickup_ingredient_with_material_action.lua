local IngredientList = require 'components.workshop.ingredient_list'
local Entity = _radiant.om.Entity

local PickupIngredientWithMaterial = class()
PickupIngredientWithMaterial.name = 'pickup material ingredient'
PickupIngredientWithMaterial.does = 'stonehearth:pickup_ingredient'
PickupIngredientWithMaterial.args = {
   ingredient = 'table',                  -- what to get
}

PickupIngredientWithMaterial.version = 2
PickupIngredientWithMaterial.priority = 1

function PickupIngredientWithMaterial:start_thinking(ai, entity, args)
   if args.ingredient.material ~= nil then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(PickupIngredientWithMaterial)
               :execute('stonehearth:pickup_item_made_of', { material = ai.ARGS.ingredient.material })
