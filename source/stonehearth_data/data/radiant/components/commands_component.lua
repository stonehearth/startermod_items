local CommandsComponent = class()

function CommandsComponent:__init(entity)
   self._entity = entity
   self._commands = {}
end

function CommandsComponent:extend(json)
   -- not really...
   if json.commands then
      for _, uri in radiant.resources.pairs(json.commands) do
         local command = radiant.resources.load_json(uri)
         
         -- expand the json.  just for fun (it's small)
         local t = radiant.json.decode(tostring(command))
         table.insert(self._commands, t)
      end
   end
end

function CommandsComponent:__tojson()
   return radiant.json.encode(self._commands)
end

function CommandsComponent:do_command(name, ...)
   for i, command in ipairs(self._commands) do
      if command.name == name then
         local api = radiant.mods.require(command.handler)
         if api and api[command.name] then
            -- xxx: validate args...
            return api[command.name](self._entity, ...)
         end
         return
      end
   end
end

return CommandsComponent
