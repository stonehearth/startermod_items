local EquipmentComponent = class()

function EquipmentComponent:__init(entity)
   self._equipped_items = {} 
   self._injected_ais = {}
   self._injected_commands = {}
end

function EquipmentComponent:__create(entity, json)
   self._entity = entity
   assert(json)
   -- xxx, interate over all the items in the json array and equip them
end

-- expect an entity, but if you pass a uri we'll make a new one
function EquipmentComponent:equip_item(item)
   if type(item) == 'string' then
      item = radiant.entities.create_entity(item)
   end
   assert(radiant.check.is_entity(item))

   local ep = item:add_component('stonehearth:equipment_piece');
   self:_setup_item_rendering(item, ep:get_render_type())
   self:_inject_item_ai(item, ep:get_injected_ai())
   self:_inject_item_commands(item, ep:get_injected_commands())

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
   local ep = item:add_component('stonehearth:equipment_piece');
   self:_remove_item_rendering(item, ep:get_render_type())
   self:_revoke_injected_item_ai(item, ep:get_injected_ai())
   self:_revoke_injected_item_commands(item, ep:get_injected_commands())
end

--- Handles all the rendering details of how the item will be rendered on the guy equipping the item
function EquipmentComponent:_setup_item_rendering(item, render_type)
   if render_type == 'merge_with_model' then
      local render_info = self._entity:add_component('render_info')
      render_info:attach_entity(item)   
   end
end

function EquipmentComponent:_remove_item_rendering(item, render_type)
   if render_type == 'merge_with_model' then
      item:add_component('render_info'):remove_entity(item:get_uri())
   end
end

--- If the item specifies ai to be injected, inject it
function EquipmentComponent:_inject_item_ai(item, injected_ai)
   if injected_ai then
      local injected_ai_token = stonehearth.ai:inject_ai(self._entity, injected_ai, item)
      self._injected_ais[item] = injected_ai_token
   end
end

function EquipmentComponent:_revoke_injected_item_ai(item)
   local injected_ai_token = self._injected_ais[item]
   if injected_ai_token then
      injected_ai_token:destroy()
      self._injected_ais[item] = nil
   end
end

function EquipmentComponent:_inject_item_commands(item, commands)
   if commands then
      local command_component = self._entity:add_component('stonehearth:commands')
      for i, command in ipairs(commands) do
         local json = radiant.resources.load_json(command)
         table.insert(self._injected_commands, json.name)
         command_component:add_command(command)
      end
   end
end

function EquipmentComponent:_revoke_injected_item_commands(commands)
   local command_component = self._entity:get_component('stonehearth:commands')

   if command_component then
      for i, command_name in ipairs(self._injected_commands) do
         command_component:remove_command(command_name)
      end
   end

   self._injected_commands = {}
end


function EquipmentComponent:destroy()
   -- xxx, revoke injected ais for all equipped items
   --stonehearth.ai:revoke_injected_ai(self._injected_ai_token)
end


return EquipmentComponent