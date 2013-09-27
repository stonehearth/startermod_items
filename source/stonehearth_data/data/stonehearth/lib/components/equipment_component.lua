local Equipment = class()

function Equipment:__init(entity)
   self._entity = entity
   self._equipped_items = {} 
   self._injected_ais = {}
end

function Equipment:extend(json)
   assert(json)
   -- xxx, interate over all the items in the json array and equip them
end

-- expect an entity, but if you pass a uri we'll make a new one
function Equipment:equip_item(item)
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
function Equipment:unequip_item(uri)
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

function Equipment:_remove_item(item)
   local item_info = item:add_component('stonehearth:equipment_piece'):get_info();
   self:_remove_item_rendering(item, item_info)
   self:_revoke_injected_item_ai(item, item_info)
end

--- Handles all the rendering details of how the item will be rendered on the guy equipping the item
function Equipment:_setup_item_rendering(item, item_info)
   if item_info.render_type == 'merge_with_model' then
      local render_info = self._entity:add_component('render_info')
      render_info:attach_entity(item)   
   end
end

function Equipment:_remove_item_rendering(item, item_info)
   if item_info.render_type == 'merge_with_model' then
      item:add_component('render_info'):remove_entity('stonehearth', 'worker_outfit')
   end
end

--- If the item specifies ai to be injected, inject it
function Equipment:_inject_item_ai(item, item_info)
   if item_info.injected_ai then
      local injected_ai_token = radiant.ai.inject_ai(self._entity, item_info.injected_ai, item)
      self._injected_ais[item] = injected_ai_token
   end   
end

function Equipment:_revoke_injected_item_ai(item, item_info)
   local injected_ai_token = self._injected_ais[item]

   if injected_ai_token then
      radiant.ai.revoke_injected_ai(injected_ai_token)
   end
end

function Equipment:destroy()
   -- xxx, revoke injected ais for all equipped items
   --radiant.ai.revoke_injected_ai(self._injected_ai_token)
end


return Equipment