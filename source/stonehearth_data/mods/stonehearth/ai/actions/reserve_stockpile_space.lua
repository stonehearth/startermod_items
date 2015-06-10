local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity

local ReserveStockpileSpace = class()
ReserveStockpileSpace.name = 'wait for stockpile space'
ReserveStockpileSpace.does = 'stonehearth:wait_for_stockpile_space'
ReserveStockpileSpace.args = {
   stockpile = Entity,        -- the stockpile that needs stuff
}
ReserveStockpileSpace.think_output = {
   item_filter = 'function',  -- the filter function for stockpile items
}
ReserveStockpileSpace.version = 2
ReserveStockpileSpace.priority = 1

function ReserveStockpileSpace:start_thinking(ai, entity, args)
   self._ai = ai
   self._log = ai:get_log()
   self._stockpile = args.stockpile:get_component('stonehearth:stockpile')
   self._storage = args.stockpile:get_component('stonehearth:storage')
   self._ready = false

   self._space_listener = radiant.events.listen(args.stockpile, 'stonehearth:stockpile:space_available', self, self._on_space_available)
   self:_on_space_available(self._stockpile, not self._stockpile:is_full())
end

function ReserveStockpileSpace:_on_space_available(stockpile, space_available)
   if not stockpile or not stockpile:get_entity():is_valid() then
      self._log:warning('stockpile destroyed')
      return
   end

   if space_available and not self._ready then
      self._ready = true
      local filter_fn = self._storage:get_filter_function()
      self._ai:set_think_output({
         item_filter = filter_fn
      })
   elseif not space_available and self._ready then
      self._ready = false
      self._ai:clear_think_output()
   end
end

function ReserveStockpileSpace:stop_thinking(ai, entity)
   self._space_listener:destroy()
   self._space_listener = nil
end

return ReserveStockpileSpace
