local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local WaitForClosestStockpileSpace = class()
WaitForClosestStockpileSpace.name = 'wait for closest stockpile space'
WaitForClosestStockpileSpace.does = 'stonehearth:wait_for_closest_stockpile_space'
WaitForClosestStockpileSpace.args = {
   item = Entity
}
WaitForClosestStockpileSpace.think_output = {
   stockpile = Entity
}
WaitForClosestStockpileSpace.version = 2
WaitForClosestStockpileSpace.priority = 1

function WaitForClosestStockpileSpace:start_thinking(ai, entity, args)
   self._ai = ai
   self._log = ai:get_log()

   local stockpiles = stonehearth.inventory:get_inventory(entity)
                                                :get_all_stockpiles()

   local shortest_distance
   local closest_stockpile

   --Iterate through the stockpiles. If we match the stockpile criteria AND there is room
   --check if the stockpile is the closest such stockpile to the entity
   for id, stockpile_entity in pairs(stockpiles) do
      local stockpile_component = stockpile_entity:get_component('stonehearth:stockpile')
      if not stockpile_component:is_full() and stockpile_component:can_stock_entity(args.item) then
         local distance_between = radiant.entities.distance_between(entity, stockpile_entity)
         if not closest_stockpile or distance_between < shortest_distance then
            closest_stockpile = stockpile_entity
            shortest_distance = distance_between
         end
      end
   end

   --If there is a closest stockpile, return it
   if closest_stockpile then
      self._ai:set_think_output({
         stockpile = closest_stockpile
      })
   else
      return
   end
end

return WaitForClosestStockpileSpace