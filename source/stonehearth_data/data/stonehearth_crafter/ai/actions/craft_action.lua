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
   local crafter_component = entity:get_component('mod://stonehearth_crafter/components/crafter_component.lua')
   local workshop = crafter_component:get_workshop()

   while workshop:work_on_curr_recipe(ai) do
   end
   ai:execute('stonehearth_crafter.activities.fill_outbox')
end

return CraftAction