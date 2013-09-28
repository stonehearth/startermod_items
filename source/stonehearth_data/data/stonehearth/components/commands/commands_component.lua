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
         self:add_command(radiant.resources.load_json(uri))
      end
   end
end

function CommandsComponent:add_command(json)
   local t = self:_replace_variables(json)
   self:_set_defaults(t)

   table.insert(self._data.commands, t)
   self._data_binding:mark_changed()
end

function CommandsComponent:modify_command(name)
   local command = self:_find_command_by_name(name)
   if command then
      self._data_binding:mark_changed()
   end
   return command
end

--Set the runtime properties like enabled or tooltip
function CommandsComponent:_set_defaults(data_table)
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
      if res == '{{self}}' then
         res = self._entity
      end
   end
   return res
end

--Given the name of a command, set its enabled/disabled status
-- xxx: ideally the only thing that would ever change about a command
-- is the enable bit, and the ui would set the tooltip based on 
-- that.
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
