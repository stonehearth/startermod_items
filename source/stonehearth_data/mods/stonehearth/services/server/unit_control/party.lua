local Point3 = _radiant.csg.Point3

local Party = class()
local PartyGuardAction = require 'services.server.unit_control.actions.party_guard_action'

local DX = -6

function Party:initialize(unit_controller, id, ord)
   self._sv._next_id = 2
   self._sv.id = id
   self._sv.name = string.format('Party No.%d', ord) -- i18n hazard. =(
   self._sv.unit_controller = unit_controller
   self._sv.commands = {}
   self._sv.members = {}
end

function Party:get_id()
   return self._sv.id
end

function Party:add_member(member)
   local id = member:get_id()
   local pc = member:add_component('stonehearth:party_member')
   local old_party = pc:get_party()
   if old_party then
      old_party:remove_member(id)
   end
   pc:set_party(self)
   
   local party_abilities = radiant.entities.create_entity('stonehearth:party:party_abilities')

   local party_task = member:get_component('stonehearth:ai')
                           :get_task_group('stonehearth:party')
                              :create_task('stonehearth:follow_party_orders', { party = self })
                                 :start()

   member:add_component('stonehearth:equipment')
            :equip_item(party_abilities)

   self._sv.members[id] = {
      entity = member,
      party_task = party_task,
      party_abilities = party_abilities,
      formation_offset = Point3(DX, 0, 0)
   }
   DX = DX + 3

   self.__saved_variables:mark_changed()
end

function Party:remove_member(id)
   local entry = self._sv.members[id]
   if entry then
      local member = entry.entity
      if member and member:is_valid() then
         member:add_component('stonehearth:party_member')
                  :set_party(nil)
      end
      self._sv.members[id] = nil
      self.__saved_variables:mark_changed()
   end
end

function Party:get_formation_location_for(member)
   local location = self._sv.party_location
   if not location then
      return
   end

   local id = member:get_id()
   local entry = self._sv.members[id]
   if not entry then
      return
   end

   return entry.formation_offset + location
end

function Party:create_command(action, target)
   local cmd_id = self:_get_next_id()
   local cmd = radiant.create_controller('stonehearth:unit_control:party_command', self, cmd_id, action, target)
   self._sv.commands[cmd_id] = cmd
   self.__saved_variables:mark_changed()
   return cmd
end

function Party:_get_next_id()
   local id = self._sv._next_id
   self._sv._next_id = id + 1
   return id
end

function Party:_start_command(cmd)
   assert(not self._sv.current_command)
   self._sv.current_command = cmd
   self._sv.party_location = cmd:get_target()
   radiant.events.trigger_async(self, 'stonehearth:party:formation_changed')
   self.__saved_variables:mark_changed()
end

function Party:set_name_command(session, response, name)
   self._sv.name = name
   self.__saved_variables:mark_changed()
   return true
end

function Party:add_member_command(session, response, member)
   self:add_member(member)
   return true
end

function Party:remove_member_command(session, response, member)
   self:remove_member(member:get_id())
   return true
end


return Party
