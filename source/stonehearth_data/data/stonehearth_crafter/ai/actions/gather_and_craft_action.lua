--[[
   Gather all the materials necessary to craft the recipe.
   If the gathering is interrupted, all collected things
   stay on the workbench, but on resume, the worker evaluates
   a new recipe (and does not attempt to finish the recipe
   he was halfway through gathering).

   Pros: If we're interrupted halfway through gathering, and
   the remaining stuff needed is no longer in the world when
   we resume, we can just start a new recipe and reuse some
   of the gathered things.
   Cons: We may have gathered a bunch of things that are no longer
   useful, and that now only he can use.
]]

local GatherAndCraftAction = class()

GatherAndCraftAction.name = 'stonehearth.actions.gather_and_craft'
GatherAndCraftAction.does = 'stonehearth_crafter.activities.gather_and_craft'
GatherAndCraftAction.priority = 5

function GatherAndCraftAction:__init(ai, entity)
   self._entity = entity
end

--[[
   Called when the crafter wants to start a new recipe
   and must gather all the items.
   recipe:  recipe for the thing we're about to craft
   missing_ingredients: data about the ingredients we still need to collect
                        (some may already be on the workbench)

   TODO: finalize the inventory/all_ing api
]]
function GatherAndCraftAction:run(ai, entity, recipe, missing_ingredients)
   local crafter_component = self._entity:get_component('mod://stonehearth_crafter/components/crafter.lua')
   local workshop = crafter_component:get_workshop()

   for _, ing_data in ipairs(missing_ingredients) do
      --TODO: replace with something like workshop:get_inbox_location()
      local bench_location = RadiantIPoint3(-12, 1, -11)
      ai:execute('stonehearth.activities.pickup_item', ing_data.item)
      --TODO: calculate adjacent with pathfinder (this is a fake adjacent right now)
      ai:execute('stonehearth.activities.goto_location', RadiantIPoint3(bench_location.x, bench_location.y, bench_location.z+1))
      ai:execute('stonehearth.activities.drop_carrying', bench_location)
      workshop:add_item_to_bench(ing_data.item)
   end
   -- Once everything's gathered, then craft
   ai:execute('stonehearth_crafter.activities.craft')
end

return GatherAndCraftAction
