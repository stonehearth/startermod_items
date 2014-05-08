local StockpileArson = class()
local StockpileComponent = require 'components.stockpile.stockpile_component'
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3


StockpileArson.name = 'stockpile arson'
StockpileArson.does = 'stonehearth:stockpile_arson'
StockpileArson.args = {
   stockpile_comp = StockpileComponent,
   location = Point3
}
StockpileArson.version = 2
StockpileArson.priority = 1

function stockpile_igniter(stockpile_comp)
   local fire_effect = radiant.effects.run_effect(stockpile_comp:get_entity(), '/stonehearth/data/effects/firepit_effect')

   stonehearth.calendar:set_timer(1200,
      function ()
         -- We have to simulate the effect of fire (i.e. it doesn't actually burn anything) for now, so we'll
         -- just wait a tad and then destroy the items.

         -- TODO: why is this necessary (without the 'remove_entity', the stockpile never
         -- gets the message that it's been emptied, and so reports that it is full.)
         local to_destroy = {}
         for _, item in pairs(stockpile_comp:get_items()) do
            table.insert(to_destroy, item)
            radiant.terrain.remove_entity(item)
         end

         for _, item in pairs(to_destroy) do
            radiant.entities.destroy_entity(item)
         end
         fire_effect:stop()
      end)
end


local ai = stonehearth.ai
return ai:create_compound_action(StockpileArson)
            :execute('stonehearth:goto_location', { location = ai.ARGS.location })
            :execute('stonehearth:run_effect', { effect = 'light_fire' })
            :execute('stonehearth:call_function', { fn = stockpile_igniter, args = { ai.ARGS.stockpile_comp } })
