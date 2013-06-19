local StockpileComponent = class()

function StockpileComponent:__init(entity)
   self._entity = entity   
   self._data = {
      size  = { 0, 0 }
   }
end

function StockpileComponent:extend(json)
   if json.size then
      self:set_size(json.size)
   end
   -- not so much...
end

function StockpileComponent:is_full()
   return false
end

function StockpileComponent:set_size(size)
   self._data.size = size
   radiant.components.mark_dirty(self, self._data)
   
   local destination = self._entity:add_component('destination')
   local bounds = Cube3(Point3(0, 0, 0), Point3(size[1], 1, size[2]))
   local region = Region3(bounds)
   destination:set_region(region)
end

return StockpileComponent
