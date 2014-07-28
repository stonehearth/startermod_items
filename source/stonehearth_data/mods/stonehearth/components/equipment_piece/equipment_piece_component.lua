local EquipmentPieceComponent = class()
local log = radiant.log.create_logger('equipment_piece')

function EquipmentPieceComponent:initialize(entity, json)
   self._entity = entity
   self._json = json
   self._sv = self.__saved_variables:get_data()
   if not self._sv._injected_commands then
   	self._sv._injected_commands = {}
	end

   if not self._sv._injected_buffs then
      self._sv._injected_buffs = {}
   end

   local owner = self._sv.owner
   if owner and owner:is_valid() then
   	-- we can't be sure what gets loaded first: us or our owner.  if we get
   	-- loaded first, it's way too early to inject the ai.  if the owner got loaded
   	-- first, the 'radiant:entities:post_create' event has already been fired.
   	-- so just load up once the whole game is loaded.
      radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
            self:_inject_ai()
            self:_inject_buffs()
         end)
   end

   assert(json.render_type)
end

function EquipmentPieceComponent:destroy()
	self:unequip()
end

function EquipmentPieceComponent:equip(entity)
	self:unequip()

	self._sv.owner = entity
	self:_inject_ai()
   self:_inject_buffs()
	self:_inject_commands()	
	self:_setup_item_rendering()
	self.__saved_variables:mark_changed()
end

function EquipmentPieceComponent:unequip()
	if self._sv.owner and self._sv.owner:is_valid() then
		self:_remove_ai()
      self:_remove_buffs()
		self:_remove_commands()
		self:_remove_item_rendering()
		self._sv.owner = nil
		self.__saved_variables:mark_changed()
	end
end

function EquipmentPieceComponent:_setup_item_rendering()
	local render_type = self._json.render_type

   if render_type == 'merge_with_model' then
      local render_info = self._sv.owner:add_component('render_info')
      render_info:attach_entity(self._entity)

   elseif render_type == 'attach_to_bone' then
      local model_variant = self._json.render_info.model_variant
      if model_variant then
         local render_info = self._entity:add_component('render_info')
         render_info:set_model_variant(model_variant)
      end

      local postures = self._json.render_info.postures
      if postures then
         radiant.events.listen(self._sv.owner, 'stonehearth:posture_changed', self, self._on_posture_changed)
         self:_on_posture_changed()
      else
         self:_attach_to_bone()
      end
   end
end

function EquipmentPieceComponent:_remove_item_rendering()
   assert(self._sv.owner and self._sv.owner:is_valid())
   local render_type = self._json.render_type

   if render_type == 'merge_with_model' then
      self._sv.owner:add_component('render_info'):remove_entity(self._entity:get_uri())
   elseif render_type == 'attach_to_bone' then
      local postures = self._json.render_info.postures
      if postures then
         radiant.events.unlisten(self._sv.owner, 'stonehearth:posture_changed', self, self._on_posture_changed)
      end
      self:_remove_from_bone()
   end
end

function EquipmentPieceComponent:_on_posture_changed()
   local posture = radiant.entities.get_posture(self._sv.owner)

   -- use a set/map for this if the list gets long
   if self:_value_is_in_array(posture, self._json.render_info.postures) then
      self:_attach_to_bone()
   else
      self:_remove_from_bone()
   end
end

function EquipmentPieceComponent:_value_is_in_array(value, array)
   for _, entry_value in pairs(array) do
      if entry_value == value then
         return true
      end
   end
   return false
end

function EquipmentPieceComponent:_attach_to_bone()
   local entity_container = self._sv.owner:add_component('entity_container')
   local bone_name = self._json.render_info.default_bone
   log:debug('%s attaching %s to bone %s', self._sv.owner, self._entity, bone_name)
   entity_container:add_child_to_bone(self._entity, bone_name)
end

function EquipmentPieceComponent:_remove_from_bone()
   local entity_container = self._sv.owner:add_component('entity_container')
   local bone_name = self._json.render_info.default_bone
   log:debug('%s detaching item on bone %s', self._sv.owner, self._entity, bone_name)
   entity_container:remove_child(self._entity:get_id())
end

function EquipmentPieceComponent:_inject_ai()
   assert(self._sv.owner)

   if self._json.injected_ai then
      self._injected_ai = stonehearth.ai:inject_ai(self._sv.owner, self._json.injected_ai, self._entity)
   end
end

function EquipmentPieceComponent:_remove_ai()
   if self._injected_ai then
      self._injected_ai:destroy()
      self._injected_ai = nil
   end
end

function EquipmentPieceComponent:_inject_buffs()
   assert(self._sv.owner)

   if self._json.injected_buffs then
      for _, buff in ipairs(self._json.injected_buffs) do
         radiant.entities.add_buff(self._entity, buff);
      end
   end
end

function EquipmentPieceComponent:_remove_buffs()
   if self._json.injected_buffs then
      for _, buff in ipairs(self._json.injected_buffs) do
         radiant.entities.remove_buff(self._entity, buff);
      end
   end
end

function EquipmentPieceComponent:_inject_commands()
   assert(self._sv.owner)
   assert(#self._sv._injected_commands == 0)

   if self._json.injected_commands then
	   local command_component = self._sv.owner:add_component('stonehearth:commands')
	   for _, uri in ipairs(self._json.injected_commands) do
	      local command = command_component:add_command(uri)
			table.insert(self._sv._injected_commands, command.name)
	   end
	end
end

function EquipmentPieceComponent:_remove_commands()
	assert(self._sv.owner and self._sv.owner:is_valid())

   if #self._sv._injected_commands > 0 then
	   local command_component = self._sv.owner:add_component('stonehearth:commands')
	   for _, command_name in ipairs(self._sv._injected_commands) do
	      command_component:remove_command(command_name)
	   end
		self._sv._injected_commands = {}
	end
end

return EquipmentPieceComponent
