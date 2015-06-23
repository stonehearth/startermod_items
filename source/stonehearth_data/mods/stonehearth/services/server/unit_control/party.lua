local Point3 = _radiant.csg.Point3

local Party = class()
local PartyGuardAction = require 'services.server.unit_control.actions.party_guard_action'

local SPACE = 2

local FORMATIONS = {
   { Point3(0, 0, 0) },
   { Point3(-SPACE, 0, 0), Point3(SPACE, 0, 0) },
   { Point3(-SPACE, 0, 0), Point3(SPACE, 0, 0), Point3(0, 0, SPACE * 2) },
}

Party.ATTACK = 'attack'
Party.DEFEND = 'defend'

local function ToPoint3(pt)
   return pt and Point3(pt.x, pt.y, pt.z) or nil
end

function Party:initialize(unit_controller, player_id, id, ord)
   self._sv._next_id = 2
   self._sv.id = id
   self._sv.name = string.format('Party No.%d', ord) -- i18n hazard. =(
   self._sv.unit_controller = unit_controller
   self._sv.members = {}
   self._sv.banners = {}
   self._sv.party_size = 0
   self._sv.leash_range = 20
   self._sv.player_id = player_id
   self._party_tasks = {}
end

function Party:activate()
   local player_id = self._sv.unit_controller:get_player_id()
   local scheduler = stonehearth.tasks:create_scheduler(player_id)   

   self._party_tg = scheduler:create_task_group('stonehearth:combat', {})
   for _, entry in pairs(self._sv.members) do
      self._party_tg:add_worker(entry.entity)
   end

   self._party_priorities = stonehearth.constants.priorities.party

   self._town = stonehearth.town:get_town(self._sv.player_id)
   self:_create_listeners()
   self:_update_party()
end

function Party:destroy()
   local id = next(self._sv.members)
   while id ~= nil do
      self:remove_member(id)
      id = next(self._sv.members)
   end

   self:_destroy_listeners()

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
      entity = member
   }
   self._sv.party_size = self._sv.party_size + 1

   self.__saved_variables:mark_changed()
   radiant.events.trigger_async(self, 'stonehearth:party:banner_changed')
end

function Party:remove_member(id)
   local entry = self._sv.members[id]
   if entry then
      self._sv.members[id] = nil
      self._sv.party_size = self._sv.party_size - 1
      self.__saved_variables:mark_changed()
      
      local member = entry.entity
      if member and member:is_valid() then        
         member:add_component('stonehearth:equipment')
                  :unequip_item('stonehearth:party:party_abilities')
         member:add_component('stonehearth:party_member')
                  :set_party(nil)
         self._party_tg:remove_worker(member:get_id())
      end
   end
end

function Party:each_member()
   local id, entry
   return function()
      id, entry = next(self._sv.members, id)
      return id, (entry and entry.entity)
   end
end

function Party:get_formation_offset(member)
   local i = 0
   local member_id = member:get_id()
   for id, _ in pairs(self._sv.members) do
      if id == member_id then
         break
      end
      i = i + 1
   end

   local formation = FORMATIONS[self._sv.party_size]
   if formation then
      local location = formation[i + 1]
      assert(location)
      return location
   end
   local w = math.ceil(math.sqrt(self._sv.party_size))
   local x = math.floor(i / w)
   local z = (i - (x * w))
   if math.mod(w, 2) == 0 then
      -- even
      x = x - (w / 2)
      z = z - (w / 2)
      return Point3((x * SPACE * 2) + SPACE, 0, (z * SPACE * 2) + SPACE)
   end
   -- odd
   x = x - math.floor(w / 2)
   z = z - math.floor(w / 2)
   return Point3(x * SPACE * 2, 0, z * SPACE * 2)
end

function Party:get_active_banner()
   local banner
   if self._town:worker_combat_enabled() then
      banner = self._sv.banners[Party.DEFEND]
   end
   if not banner then
      banner = self._sv.banners[Party.ATTACK]
   end
   return banner
end


function Party:cancel_tasks()
   if self._party_tasks then 
      for _, task in pairs(self._party_tasks) do
         task:destroy()
      end
   end
   self._party_tasks = {}
end

function Party:add_task(activity, args)
   local task = self._party_tg:create_task(activity, args)
                              :set_priority(stonehearth.constants.priorities.combat.PARTY_TASK)

   table.insert(self._party_tasks, task)
   return task
end

function Party:_get_next_id()
   local id = self._sv._next_id
   self._sv._next_id = id + 1
   return id
end

function Party:_change_banner(type, location, movement_type)
   checks('self', 'string', 'Point3', 'string')
   self._sv.banners[type] = {
      type = type,
      location = location,
      movement_type = movement_type,
   }
   self.__saved_variables:mark_changed()   
   self:_update_party()
end

function Party:place_banner(type, location, rotation)
   -- legacy banner stuff
   self:_change_banner(type, location, 'passive')
end

function Party:attack_move_to(location)
   self:_change_banner(Party.ATTACK, location, 'aggressive')
end

function Party:remove_banner(type)
   self._sv.banners[type] = nil
   self.__saved_variables:mark_changed()
   self:_update_party()

   radiant.events.trigger_async(self, 'stonehearth:party:banner_changed', {
         type = type
      })
end

function Party:_update_party()
   self:_update_tasks()
   self:_update_leashes()
end

function Party:_update_tasks()
   if self._movement_task then
      self._movement_task:destroy()
      self._movement_task = nil
   end

   local banner = self:get_active_banner()   
   if banner then
      local priority = 1
      local activity = 'stonehearth:party:move_to_banner'
      local args = {
         party = self,
         location = banner.location,
      }

      --PASSIVE IS BROKEN! TODO: FIGURE OUT WHY! TILL THEN, EVERYONE IS AGGRESSIVE
      if banner.movement_type == 'passive' then
         priority = stonehearth.constants.priorities.combat.PARTY_AGGRESSIVE_FORMATION
      else
         priority = stonehearth.constants.priorities.combat.PARTY_AGGRESSIVE_FORMATION
      end
      --radiant.log.write('', 0, 'priority is %d', priority)
      self._movement_task = self._party_tg:create_task(activity, args)
                                                   :set_priority(priority)
                                                   :start()
   end
end

function Party:_update_leashes()
   local banner = self:get_active_banner()
   for _, entry in pairs(self._sv.members) do
      local cs = entry.entity:add_component('stonehearth:combat_state')

      if entry.leash then
         entry.leash:destroy()
         entry.leash = nil
      end
      if banner and banner.movement_type == 'passive' then
         cs:set_attack_leash(banner.location, self._sv.leash_range)
      else
         cs:remove_attack_leash()
      end
   end
end

function Party:_create_listeners()
   if not self._town_listener then
      self._town_listener = radiant.events.listen(self._town, 'stonehearth:town_defense_mode_changed', self, self._check_town_defense_mode)
   end
end

function Party:_destroy_listeners()
   if self._town_listener then
      self._town_listener:destroy()
      self._town_listener = nil
   end
end

function Party:_check_town_defense_mode()
   self:_update_party()
   radiant.events.trigger_async(self, 'stonehearth:party:banner_changed', {
         type = 'defend'
      })   
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

function Party:place_banner_command(session, response, type, location, rotation)
   self:place_banner(type, ToPoint3(location), rotation)
   return true
end

function Party:remove_banner_command(session, response, type)
   self:remove_banner(type)
   return true
end

return Party
