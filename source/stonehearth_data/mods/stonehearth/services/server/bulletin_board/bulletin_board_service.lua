local log = radiant.log.create_logger('bulletin')

BulletinBoardService = class()

-- should we make this part of Town?
-- that way we don't have to do this per-player bookkeeping
function BulletinBoardService:initialize()
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.datastores = {}
      self._sv.bulletin_to_player_map = {}
      self._sv.next_bulletin_id = 100 -- can't use 1 because we want bulletins to serialize as a map
      self._sv.initialized = true
      self.__saved_variables:mark_changed()
   end
end

function BulletinBoardService:destroy()
end

-- do we still need this?
function BulletinBoardService:update(bulletin_id)
   local player_id = self._sv.bulletin_to_player_map[bulletin_id]
   local datastore = self:get_datastore(player_id)
   local bulletins = datastore:get_data().bulletins
   datastore:mark_changed()
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

   self.__saved_variables:mark_changed()
   return bulletin
end

-- Removes all bulletins tracked by the service.
-- Currently used by autotests to ensure bulletin board sane-ness.
function BulletinBoardService:remove_all_bulletins(player_id)
   for bulletin_id, player_id in pairs (self._sv.bulletin_to_player_map) do
      self:remove_bulletin(bulletin_id)
   end
end

function BulletinBoardService:remove_bulletin(bulletin)
   local bulletin_id
   if type(bulletin) == 'number' then
      bulletin_id = bulletin
   else
      bulletin_id = bulletin:get_id()
   end

   local player_id = self._sv.bulletin_to_player_map[bulletin_id]

   if not player_id then
      -- already removed, nothing to do
      return
   end

   local datastore = self:get_datastore(player_id)
   local bulletins = datastore:get_data().bulletins

   bulletins[bulletin_id]:destroy()
   bulletins[bulletin_id] = nil
   self._sv.bulletin_to_player_map[bulletin_id] = nil
   datastore:mark_changed()
   self.__saved_variables:mark_changed()
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
      datastore:mark_changed()
      self.__saved_variables:mark_changed()
   end

   return datastore
end

function BulletinBoardService:mark_shown(bulletin_id)
   local bulletin = self:get_bulletin(bulletin_id)
   bulletin:set_shown(true)
end

function BulletinBoardService:_add_bulletin(player_id, bulletin)
   local datastore = self:get_datastore(player_id)
   local bulletins = datastore:get_data().bulletins
   local bulletin_id = bulletin:get_id()

   bulletins[bulletin_id] = bulletin
   self._sv.bulletin_to_player_map[bulletin_id] = player_id
   datastore:mark_changed()
   self.__saved_variables:mark_changed()
end

return BulletinBoardService
