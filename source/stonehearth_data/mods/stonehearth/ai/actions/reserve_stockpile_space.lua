local StockpileComponent = require 'components.stockpile.stockpile_component'
local Point3 = _radiant.csg.Point3

local ReserveStockpileSpace = class()
ReserveStockpileSpace.name = 'wait for stockpile space'
ReserveStockpileSpace.does = 'stonehearth:wait_for_stockpile_space'
ReserveStockpileSpace.args = {
   stockpile = StockpileComponent,      -- the stockpile that needs stuff
}
ReserveStockpileSpace.think_output = {
   stockpile = StockpileComponent,     -- the stockpile that needs stuff
   item_filter = 'function',           -- the filter function for stockpile items
}
ReserveStockpileSpace.version = 2
ReserveStockpileSpace.priority = 1

function ReserveStockpileSpace:start_thinking(ai, entity, args)
   self._ai = ai
   self._log = ai:get_log()
   self._stockpile = args.stockpile
   self._ready = false

   radiant.events.listen(self._stockpile, 'space_available', self, self._on_space_available)
   self:_on_space_available(self._stockpile, not self._stockpile:is_full())
end

function ReserveStockpileSpace:_on_space_available(stockpile, space_available)
   if not stockpile or not stockpile:get_entity():is_valid() then
      self._log:warning('stockpile destroyed')
      return
   end

   if space_available and not self._ready then
      self._ready = true
      local filter_fn = self._stockpile:get_filter()
      self._ai:set_think_output({
         stockpile = self._stockpile,
         item_filter = filter_fn
      })
   elseif not space_available and self._ready then
      self._ready = false
      self._ai:clear_think_output()
   end
end

function ReserveStockpileSpace:stop_thinking(ai, entity)
   radiant.events.unlisten(self._stockpile, 'space_available', self, self._on_space_available)
end

return ReserveStockpileSpace
