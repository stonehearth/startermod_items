--[[
   Create a new instance of this tracker whenever you need to keep track of
   how many you have of a certain object. 
]]

local inventory_service = stonehearth.inventory

local QuantityTracker = class()

--- A mechaism for tracking how many of a thing you have
--  @param faction - the faction this tracker belongs to
--  @param filter_fn - function takes and item and returns true if useful to the tracker, false otherwise
--  @param identifier_fn - function takes an item and returns a unique id for its class. Can be uri, or material type, etc.
function QuantityTracker:__init(faction, filter_fn, identifier_fn)
   self.__savestate = radiant.create_datastore()
   self._data = self.__savestate:get_data()
   self._inventory = inventory_service:get_inventory(faction)

   self._data.tracked_items = {}
   self._filter_fn = filter_fn
   self._identifier_fn = identifier_fn

   radiant.events.listen(self._inventory, 'stonehearth:item_added', self, self.on_item_added)
   radiant.events.listen(self._inventory, 'stonehearth:item_removed', self, self.on_item_removed)
end

function QuantityTracker:get__savestate()
   return self.__savestate;
end

function QuantityTracker:get_quantity(identifier)
   local data = self._data.tracked_items[identifier]
   if data then
      return data.count
   end
   return nil
end

function QuantityTracker:on_item_added(e)
   assert(e.item, "Quantity Tracker: The event has been fired but the item no longer exists")
   local unit_info = e.item:get_component('unit_info')
   local identifier, user_visible_name, icon = self._identifier_fn(e.item)

   if self._filter_fn(e.item) and unit_info and identifier then
      if self._data.tracked_items[identifier] then
         self._data.tracked_items[identifier].count = self._data.tracked_items[identifier].count + 1
      else
         if icon == nil then
            icon = unit_info:get_icon()
         end
         if user_visible_name == nil then
            user_visible_name = unit_info:get_display_name()
         end
         self._data.tracked_items[identifier] = {
            name = user_visible_name,
            icon = icon, 
            count = 1
         }
      end
      self.__savestate:mark_changed()
   end
end

function QuantityTracker:on_item_removed(e)
   assert(e.item, "Quantity Tracker: The item has been removed but whe don't know what that item is")
   local identifier = self._identifier_fn(e.item)
   local item_data = self._data.tracked_items[identifier]
   if item_data then
      item_data.count = item_data.count - 1
      self.__savestate:mark_changed()
   end
end

return QuantityTracker