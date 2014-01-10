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
   self._stockpile = args.stockpile
   
   radiant.events.listen(self._stockpile, 'space_available', self, self._on_space_available)
   self:_on_space_available(not self._stockpile:is_full())
end

function ReserveStockpileSpace:_on_space_available(stockpile, space_available)
   if space_available then
      self._ai:set_think_output({
         stockpile = self._stockpile,
         item_filter = self._stockpile:get_item_filter_fn()
      })
   else
      self._ai:clear_think_output()
   end
end

function ReserveStockpileSpace:stop_thinking(ai, entity)
   radiant.events.unlisten(self._stockpile, 'space_available', self, self._on_space_available)
end

return ReserveStockpileSpace
