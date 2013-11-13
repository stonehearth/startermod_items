local inventory_service = require 'services.inventory.inventory_service'

local ResourceTracker = class()

function ResourceTracker:__init(faction)
   self._data_store = _radiant.sim.create_data_store()
   self._data = self._data_store:get_data()
   self._inventory = inventory_service:get_inventory(faction)

   self._data.resource_types = {}

   radiant.events.listen(self._inventory, 'stonehearth:item_added', self, self.on_item_added)
   radiant.events.listen(self._inventory, 'stonehearth:item_removed', self, self.on_item_removed)
end

function ResourceTracker:get_data_store()
   return self._data_store;
end

function ResourceTracker:on_item_added(e)
   assert(e.item)

   local material = e.item:get_component('stonehearth:material')
   local unit_info = e.item:get_component('unit_info')

   --TODO: Sort by faction!!!
   
   if material and material:is('resource') and unit_info then
      local name = unit_info:get_display_name()
      local uri = e.item:get_uri()
      if self._data.resource_types[uri] then 
         self._data.resource_types[uri].count = self._data.resource_types[uri].count + 1
      else
         local icon = unit_info:get_icon()
         self._data.resource_types[uri] = {
            name = name,
            icon = icon,
            count = 1
         }
      end

      self._data_store:mark_changed()
   end
end

function ResourceTracker:on_item_removed(e)
   assert(e.item)

   local uri = e.item:get_uri()
   local item_data = self._data.resource_types[uri]

   if item_data then
      item_data.count = item_data.count - 1
      self._data_store:mark_changed()
   end


end

return ResourceTracker
