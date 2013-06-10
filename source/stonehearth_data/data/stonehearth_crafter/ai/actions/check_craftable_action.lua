--[[
   At some interval, check if the crafter should be crafting anything.
   If the worker's todo list has something that can be 
   crafted right now, or if the last thing we tried to craft is waiting
   unfinished ont he bench, change the priority of this component to be
   higher (5) and call the appropriate gather or craft action. 
]]

local CheckCraftableAction = class() 

CheckCraftableAction.name = 'stonehearth.actions.check_craftable'
CheckCraftableAction.does = 'stonehearth.activities.top'
CheckCraftableAction.priority = 0

function CheckCraftableAction:__init(ai, entity)
   radiant.check.is_entity(entity)  
   self._entity = entity         --the crafter
   self._ai = ai

   self._is_crafting = false     --whether we're currently actively crafting something
   self._next_activity = {}      --the thing we shoud be doing next (gathering/crafting)

   radiant.events.listen('radiant.events.gameloop', self)
end

--[[
   On gameloop (TODO: replace with on inventory added, or something)
   check if the worker associated with this action:

   a.) is currently in the middle of crafting something
   b.) was crafting but got interrupted
   c.) can craft something new off the todo list

   If a.), return.
   If b.) tell the worker to resume crafting. 
   If c.), start gathering materials for crafting. 
]]
CheckCraftableAction['radiant.events.gameloop'] = function(self)
   if self._is_crafting then 
      return
   end

   local crafter_component = self._entity:get_component('mod://stonehearth_crafter/components/crafter.lua')
   local workshop = crafter_component:get_workshop()
   local todo_list = workshop:get_todo_list()

   if workshop:get_intermediate_item() then
      self._is_crafting = true
      self._next_activity = {'stonehearth_crafter.activities.craft'}
      self._ai:set_action_priority(self, 5)
   elseif workshop:has_bench_outputs() then
      self._is_crafting = true
      self._next_activity = {'stonehearth_crafter.activities.fill_outbox'}
      self._ai:set_action_priority(self, 5)
   else
      local order, ingredient_data = todo_list:get_next_task()
      if order then
         self._is_crafting = true
         workshop:set_curr_order(order)
         self._next_activity = {'stonehearth_crafter.activities.gather_and_craft', order:get_recipe(), ingredient_data}
         self._ai:set_action_priority(self, 5)
      else
         --there is no recipe
         self._ai:set_action_priority(self, 0)
      end
   end
end

--[[
   When the action comes up on the priority lottery,
   call the actual craft action with the next activity 
   and relevant arguments
]]
function CheckCraftableAction:run(ai, entity)
   ai:execute(unpack(self._next_activity))
   self._is_crafting = false
   self._ai:set_action_priority(self, 0)
end

--[[
   If any of the actions spawned by CheckCraftable are
   stopped, be sure to remember that we're not currently
   crafting (so the next check can act appopriately)
]]
function CheckCraftableAction:stop()
   self._is_crafting = false
end

return CheckCraftableAction