local Party = class()

function Party:initialize(unit_controller, id)
   self._sv.id = id
   self._sv.next_id = 2
   self._sv.unit_controller = unit_controller
   self._sv.commands = {}
   self._sv.members = {}
end

function Party:get_id()
   return self._sv.id
end

function Party:add_member(member)
   local id = member:get_id()
   self._sv.members[id] = member
   self.__saved_variables:mark_changed()
end

function Party:create_command(action, target)
   local cmd_id = self:_get_next_id()
   local cmd = radiant.create_controller('stonehearth:unit_control:party_command', self, cmd_id, action, target)
   self._sv.commands[cmd_id] = cmd
   self.__saved_variables:mark_changed()
   return cmd
end

function Party:_get_next_id()
   local id = self._sv.next_id
   self._sv.next_id = id + 1
   return id
end

function Party:_start_command(cmd)
   assert(not self._sv.current_command)
   self._sv.current_command = cmd
   self.__saved_variables:mark_changed()

   self._travel_tasks = {}
   for id, member in pairs(self._sv.members) do
      local task = member:get_component('stonehearth:ai')
                           :get_task_group('stonehearth:party_commands')
                              :create_task('stonehearth:goto_location', { 
                                       reason = 'party command',
                                       location = cmd:get_target(),
                                    })
                                 :notify_completed(function()
                                       self:_on_arrived(id, member)
                                    end)
                                 :once()
                                 :start()
      self._travel_tasks[id] = task
   end

   return self
end

function Party:_on_arrived(id, member)
   self._travel_tasks[id] = nil      
end

return Party
