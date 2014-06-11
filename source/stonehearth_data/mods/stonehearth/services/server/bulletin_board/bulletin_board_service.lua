local log = radiant.log.create_logger('bulletin')

BulletinBoardService = class()

function BulletinBoardService:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.datastores = {}
      self._sv.bulletin_to_player_map = {}
      self._sv.next_bulletin_id = 100 -- can't use 1 because we want bulletins to serialize as a map
      self._sv.initialized = true
   end
end

function BulletinBoardService:destroy()
end

function BulletinBoardService:post_bulletin(player_id)
   local bulletin = radiant.create_controller('stonehearth:bulletin', self._sv.next_bulletin_id)
   self._sv.next_bulletin_id = self._sv.next_bulletin_id + 1

   self:_add_bulletin(player_id, bulletin)

   --For autotest purposes, trigger an event
   radiant.events.trigger_async(stonehearth.bulletin_board, 'stonehearth:trigger_bulletin_for_test', {
         bulletin = bulletin, 
         time = stonehearth.calendar:get_time_and_date()
      })

   return bulletin
end

function BulletinBoardService:remove_bulletin(bulletin_id)
   local player_id = self._sv.bulletin_to_player_map[bulletin_id]
   local datastore = self:get_datastore(player_id)
   local bulletins = datastore:get_data().bulletins

   bulletins[bulletin_id] = nil
   datastore:mark_changed()
end

function BulletinBoardService:get_bulletin(bulletin_id)
   local player_id = self._sv.bulletin_to_player_map[bulletin_id]
   local datastore = self:get_datastore(player_id)
   local bulletins = datastore:get_data().bulletins

   return bulletins[bulletin_id]
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
   local bulletin_id = bulletin:get_id()

   bulletins[bulletin_id] = bulletin
   self._sv.bulletin_to_player_map[bulletin_id] = player_id
   datastore:mark_changed()
end

return BulletinBoardService
