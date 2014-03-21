local log = radiant.log.create_logger('commands')

local CommandsComponent = class()

function CommandsComponent:__init(entity, data_binding)
   --self._commands = {}
   self._commands = {
      n = 0,
   }
end

function CommandsComponent:initialize(entity, json)
   self._entity = entity
   self.__saved_variables = radiant.create_datastore({
      commands = self._commands
   })

   if json.commands then
      for _, uri in pairs(json.commands) do
         self:add_command(uri)
      end
   end
end

function CommandsComponent:restore(entity, saved_variables)
   self.__saved_variables = saved_variables
   self._commands = self.__saved_variables:get_data().commands
end

function CommandsComponent:add_command(uri)
   local json = radiant.resources.load_json(uri)
   local t = self:_replace_variables(json)
   self:_set_defaults(t)

   table.insert(self._commands, t)
   self.__saved_variables:mark_changed()
   return t
end

function CommandsComponent:remove_command(name)
   for i, command in ipairs(self._commands) do
      if command.name == name then
         table.remove(self._commands, i)
         self.__saved_variables:mark_changed()
         break;
      end
   end
end

function CommandsComponent:modify_command(name, cb)
   local command = self:_find_command_by_name(name)
   if command then
      cb(command)
      self.__saved_variables:mark_changed()
   else
      log:error('Cannot modify command "%s". It was not found.', name)
   end
end

--Set the runtime properties like enabled or tooltip
function CommandsComponent:_set_defaults(data_table)
   if data_table.default_enabled == nil then
      data_table.enabled = true;
   else
      data_table.enabled = data_table.default_enabled
   end

   --Tuck away the description so we'll have it around when we overwrite it as the command
   -- is enabled and disabled
   data_table.enabled_description = data_table.description

   if not data_table.enabled and data_table.disabled_description then
      data_table.description = data_table.disabled_description
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
      if status  then
         command.description = command.enabled_description
      else
         if not status and command.disabled_description then
            command.description = command.disabled_description
         end
      end
      self.__saved_variables:mark_changed()
   else
      log:error('Cannot modify command "%s". It was not found.', name)
   end
end

--[[
   Given a command's name, return the command, or nil if
   we can't find a command by that name.
]]
function CommandsComponent:_find_command_by_name(name)
   for i, command in ipairs(self._commands) do
      if command.name == name then
         return command
      end
   end
   return nil
end

return CommandsComponent
