--[[
   The craft action take a set of ingredients and a recipe and 
   produces the item in the recipe. This action assumes all the
   ingredients are on the workbench table. If this activity is
   interrupted, it will resume progress at the next craft check.
]]
local CraftAction = class()

CraftAction.name = 'stonehearth.actions.craft'
CraftAction.does = 'stonehearth_crafter.activities.craft'
CraftAction.priority = 5

--[[
   If the workshop doesn't yet have an intermediate item (indicating that
   crafting has started) create one. Then, for every work unit specified
   by the item's recipe, increment the progress on the intermediate item
   till the item is complete. Fill the outbox and let the workshop know 
   the item is complete
]]
function CraftAction:run(ai, entity)
   local crafter_component = entity:get_component('mod://stonehearth_crafter/components/crafter.lua')
   local workshop = crafter_component:get_workshop()
   local intermediate_item = workshop:get_intermediate_item()

   if not intermediate_item then
      intermediate_item = workshop:create_intemediate_item()
   end

   --while WorkUnits < recipe's units, 
   local work_units = workshop:get_curr_recipe().work_units
   while intermediate_item.progress < work_units do
      local work_effect = crafter_component:get_work_effect()
      ---[[
      --TODO: replace with ai:execute('stonehearth.activities.run_effect,''saw')
      --It does not seem to be working right now, but this does:
      self._effect = radiant.effects.run_effect(entity, 'saw')
      ai:wait_until(function()
         return self._effect:finished()
      end)
      self._effect = nil
      --]]
      intermediate_item.progress = intermediate_item.progress + 1
   end

   --fill the outbox
   --local recipe = workshop:get_curr_recipe()
   workshop:crafting_complete()
   ai:execute('stonehearth_crafter.activities.fill_outbox')   
end

return CraftAction