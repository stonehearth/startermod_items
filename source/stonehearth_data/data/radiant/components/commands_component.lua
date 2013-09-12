local CommandsComponent = class()

function CommandsComponent:__init(entity, data_binding)
   self._entity = entity
   --self._commands = {}

   self._data = data_binding:get_data()
   self._data.commands = {}
   self._data_binding = data_binding
   self._data_binding:mark_changed()
end

function CommandsComponent:extend(json)
   -- not really...
   if json.commands then
      for _, uri in pairs(json.commands) do
         local command = radiant.resources.load_json(uri)

         -- expand the json. just for fun (it's small)
         local t = self:_replace_variables(command)
         t = self:_set_enabled_defaults(t)

         table.insert(self._data.commands, t)
         self._data_binding:mark_changed()
      end
   end
end

--Set the runtime properties like enabled or tooltip
function CommandsComponent:_set_enabled_defaults(data_table)
   if data_table.default_enabled == nil then
      data_table.enabled = true;
   else
      data_table.enabled = data_table.default_enabled
   end

   --If no tooltip was specified, then enabled/disabled tooltips may be specified instead
   if not data_table.tooltip then
      data_table.tooltip = data_table.enabled_tooltip
      if not data_table.enabled and data_table.disabled_tooltip then
         data_table.tooltip = data_table.disabled_tooltip
      end
   end

   return data_table
end

function CommandsComponent:_replace_variables(res)
   if type(res) == 'table' then
      local result = {}
      for k, v in pairs(res) do
         result[k] = self:_replace_variables(v)
      end
      return result
   end

   if type(res) == 'string' then
      if res == '{{entity_id}}' then
         res = self._entity:get_id()
      end
   end
   return res
end

function CommandsComponent:do_command(name, ...)
   local command = self:_find_command_by_name(name)
   if command then
      local api = radiant.mods.require(command.handler)
      if api and api[command.name] then
         -- xxx: validate args...
         return api[command.name](self._entity, ...)
      end
   end
end

--Given the name of a command, set its enabled/disabled status
function CommandsComponent:enable_command(name, status)
   local command = self:_find_command_by_name(name)
   if command then
      command.enabled = status
      if status and command.enabled_tooltip then
         command.tooltip = command.enabled_tooltip
      else
         if not status and command.disabled_tooltip then
            command.tooltip = command.disabled_tooltip
         end
      end
      self._data_binding:mark_changed()
   end
end

--Given the name of a command, add or edit a field in its event_data
--A command's event_data is passed to the target of the command (ie, an event)
function CommandsComponent:add_event_data(name, key, value)
   local command = self:_find_command_by_name(name)
   if command and command.action == 'fire_event' then
      command.event_data[key] = value
   end
end

--[[
   Given a command's name, return the command, or nil if
   we can't find a command by that name.
]]
function CommandsComponent:_find_command_by_name(name)
   for i, command in ipairs(self._data.commands) do
      if command.name == name then
         return command
      end
   end
   return nil
end

return CommandsComponent
