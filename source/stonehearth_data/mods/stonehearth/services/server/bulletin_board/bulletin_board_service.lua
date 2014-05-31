local log = radiant.log.create_logger('bulletin')

BulletinBoardService = class()

function BulletinBoardService:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.datastores = {}
      self._sv.bulletin_to_player_map = {}
      self._sv.next_bulletin_id = 1
      self._sv.initialized = true
   end
end

function BulletinBoardService:destroy()
end

function BulletinBoardService:post_bulletin(player_id)
   local bulletin = radiant.create_controller('stonehearth:bulletin', self._sv.next_bulletin_id)
   self._sv.next_bulletin_id = self._sv.next_bulletin_id + 1

   self:_add_bulletin(player_id, bulletin)
   return bulletin
end

function BulletinBoardService:remove_bulletin(bulletin_id)
   local player_id = self._sv.bulletin_to_player_map[bulletin_id]
   local datastore = self:get_datastore(player_id)
   local bulletins = datastore:get_data().bulletins

   local index, bulletin = self:_find_bulletin(bulletins, bulletin_id)
   table.remove(bulletins, index)
   datastore:mark_changed()

   return bulletin
end

function BulletinBoardService:get_bulletin(bulletin_id)
   local player_id = self._sv.bulletin_to_player_map[bulletin_id]
   local datastore = self:get_datastore(player_id)
   local bulletins = datastore:get_data().bulletins

   local _, bulletin = self:_find_bulletin(bulletins, bulletin_id)
   return bulletin
end

function BulletinBoardService:trigger_event(bulletin_id, event_name, ...)
   local bulletin = self:get_bulletin(bulletin_id)
   if bulletin then
      return bulletin:trigger_event(event_name, ...)
   else
      log:error('bulletin id %d not found', bulletin_id)
   end
end

function BulletinBoardService:get_datastore(player_id)
   radiant.check.is_string(player_id)
   local datastore = self._sv.datastores[player_id]

   if not datastore then
      datastore = radiant.create_datastore()
      self._sv.datastores[player_id] = datastore
      local data = datastore:get_data()
      data.bulletins = {}
   end

   return datastore
end

function BulletinBoardService:_add_bulletin(player_id, bulletin)
   local datastore = self:get_datastore(player_id)
   local bulletins = datastore:get_data().bulletins

   table.insert(bulletins, bulletin)

   self._sv.bulletin_to_player_map[bulletin:get_id()] = player_id

   datastore:mark_changed()
end

-- O(1) search, consider making bulletins a map
function BulletinBoardService:_find_bulletin(bulletins, bulletin_id)
   for index, bulletin in pairs(bulletins) do
      if bulletin:get_id() == bulletin_id then
         return index, bulletin
      end
   end
   return nil, nil
end

return BulletinBoardService
