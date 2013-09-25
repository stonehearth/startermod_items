local Inventory = class()

function Inventory:__init(faction)
   self._faction = faction
end

function Inventory:create_stockpile(location, size)
   local entity = radiant.entities.create_entity('stonehearth_inventory.stockpile')
   
   radiant.terrain.place_entity(entity, location)
   entity:get_component('radiant:stockpile'):set_size(size)
   entity:get_component('unit_info'):set_faction(self._faction)
   -- return something?
end 

function Inventory:get_data_blob()
   return self._blob
end

return Inventory
