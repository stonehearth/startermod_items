local EquipmentPieceComponent = class()

function EquipmentPieceComponent:initialize(entity, json)
   self._entity = entity
   self._json = json
   self._sv = self.__saved_variables:get_data()
   if not self._sv._injected_commands then
   	self._sv._injected_commands = {}
	end

   local owner = self._sv.owner
   if owner and owner:is_valid() then
   	-- we can't be sure what gets loaded first: us or our owner.  if we get
   	-- loaded first, it's way too early to inject the ai.  if the owner got loaded
   	-- first, the 'radiant:entities:post_create' event has already been fired.
   	-- so just load up once the whole game is loaded.
      radiant.events.listen(radiant, 'radiant:game_loaded', function(e)
            self:_inject_ai()
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
	self:_inject_commands()	
	self:_setup_item_rendering()
	self.__saved_variables:mark_changed()
end

function EquipmentPieceComponent:unequip()
	if self._sv.owner and self._sv.owner:is_valid() then
		self:_remove_ai()
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
   end
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

function EquipmentPieceComponent:_remove_item_rendering()
	assert(self._sv.owner and self._sv.owner:is_valid())

	local render_type = self._json.render_type
   if render_type == 'merge_with_model' then
      self._sv.owner:add_component('render_info'):remove_entity(self._entity:get_uri())
   end
end

return EquipmentPieceComponent
