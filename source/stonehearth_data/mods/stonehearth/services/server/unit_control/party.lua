local Point3 = _radiant.csg.Point3

local Party = class()
local PartyGuardAction = require 'services.server.unit_control.actions.party_guard_action'

local DX = -6

local function ToPoint3(pt)
   return pt and Point3(pt.x, pt.y, pt.z) or nil
end

function Party:initialize(unit_controller, id, ord)
   self._sv._next_id = 2
   self._sv.id = id
   self._sv.name = string.format('Party No.%d', ord) -- i18n hazard. =(
   self._sv.unit_controller = unit_controller
   self._sv.members = {}

   self:restore()
end

function Party:restore()
   local player_id = self._sv.unit_controller:get_player_id()
   local scheduler = stonehearth.tasks:create_scheduler(player_id)   

   self._party_tg = scheduler:create_task_group('stonehearth:party', {})
   self._party_priorities = stonehearth.constants.priorities.party
end

function Party:destroy()
   if self._hold_formation_task then
      self._hold_formation_task:destroy()
      self._hold_formation_task = nil
   end
   if self._party_tg then
      self._party_tg:destroy()
      self._party_tg = nil
   end
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
   
   member:add_component('stonehearth:equipment')
            :equip_item('stonehearth:party:party_abilities')

   self._party_tg:add_worker(member)

   self._sv.members[id] = {
      entity = member,
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
         member:add_component('stonehearth:equipment')
                  :unequip_item('stonehearth:party:party_abilities')
         self._party_tg:remove_worker(member:get_id())
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

function Party:create_attack_order(location, rotation)
   self:_set_formation_location(location, rotation)
end

function Party:raid(stockpile)
   -- the formation should be the center of the stockpile
   local location = stockpile:get_component('stonehearth:stockpile')
                                 :get_bounds()
                                    :get_centroid()

   local task = self._party_tg:create_task('stonehearth:party:raid_stockpile', {
         party = self,
         stockpile = stockpile,
      })

   task:set_priority(self._party_priorities.RAID_STOCKPILE)
         :start()

   self:_set_formation_location(location)
end


function Party:_get_next_id()
   local id = self._sv._next_id
   self._sv._next_id = id + 1
   return id
end

function Party:_set_formation_location(location, rotation)
   if self._hold_formation_task then
      self._hold_formation_task:destroy()
      self._hold_formation_task = nil
   end

   location = radiant.terrain.get_standable_point(location)

   self._sv.party_location = location
   self.__saved_variables:mark_changed()
   radiant.events.trigger_async(self, 'stonehearth:party:formation_changed')

   self._hold_formation_task = self._party_tg:create_task('stonehearth:party:hold_formation', { party = self })
                                                :set_priority(self._party_priorities.HOLD_FORMATION)
                                                :start()
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

function Party:create_attack_order_command(session, response, location, rotation)
   self:create_attack_order(ToPoint3(location), rotation)
   return true
end

return Party
