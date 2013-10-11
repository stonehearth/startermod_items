local EquipmentComponent = class()

function EquipmentComponent:__init(entity)
   self._entity = entity
   self._equipped_items = {} 
   self._injected_ais = {}
end

function EquipmentComponent:extend(json)
   assert(json)
   -- xxx, interate over all the items in the json array and equip them
end

-- expect an entity, but if you pass a uri we'll make a new one
function EquipmentComponent:equip_item(item)
   if type(item) == 'string' then
      item = radiant.entities.create_entity(item)
   end
   assert(radiant.check.is_entity(item))

   local item_info = item:add_component('stonehearth:equipment_piece'):get_info();
   self:_setup_item_rendering(item, item_info)
   self:_inject_item_ai(item, item_info)

   table.insert(self._equipped_items, item)
end

-- expect a uri or an enitity
function EquipmentComponent:unequip_item(uri)
   if type(uri) ~= 'string' then
      uri = uri:get_uri()
   end

   local equipped_item = nil
   for i, item in pairs(self._equipped_items) do
      local item_uri = item:get_uri()
      if item_uri == uri then
         equipped_item = item
         self:_remove_item(item)
         self._equipped_items[i] = nil
      end
   end

   return equipped_item
end

function EquipmentComponent:_remove_item(item)
   local item_info = item:add_component('stonehearth:equipment_piece'):get_info();
   self:_remove_item_rendering(item, item_info)
   self:_revoke_injected_item_ai(item, item_info)
end

--- Handles all the rendering details of how the item will be rendered on the guy equipping the item
function EquipmentComponent:_setup_item_rendering(item, item_info)
   if item_info.render_type == 'merge_with_model' then
      local render_info = self._entity:add_component('render_info')
      render_info:attach_entity(item)   
   end
end

function EquipmentComponent:_remove_item_rendering(item, item_info)
   if item_info.render_type == 'merge_with_model' then
      item:add_component('render_info'):remove_entity(item:get_uri())
   end
end

--- If the item specifies ai to be injected, inject it
function EquipmentComponent:_inject_item_ai(item, item_info)
   if item_info.injected_ai then
      local ai_service = radiant.mods.load('stonehearth').ai
      local injected_ai_token = ai_service:inject_ai(self._entity, item_info.injected_ai, item)
      self._injected_ais[item] = injected_ai_token
   end   
end

function EquipmentComponent:_revoke_injected_item_ai(item, item_info)
   local injected_ai_token = self._injected_ais[item]

   if injected_ai_token then
      local ai_service = radiant.mods.load('stonehearth').ai
      ai_service:revoke_injected_ai(injected_ai_token)
   end
end

function EquipmentComponent:destroy()
   -- xxx, revoke injected ais for all equipped items
   --radiant.mods.load('stonehearth').ai:revoke_injected_ai(self._injected_ai_token)
end


return EquipmentComponent