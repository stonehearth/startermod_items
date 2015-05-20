local Entity = _radiant.om.Entity

local JobService = class()

local function _get_player_id(arg1)
   if radiant.util.is_a(arg1, 'string') then
      return arg1
   end
   if radiant.util.is_a(arg1, Entity) then
      return radiant.entities.get_player_id(arg1)
   end
   error(string.format('unexpected value %s in get_player', radiant.util.tostring(player_id)))
end

function JobService:initialize()   
   self._sv = self.__saved_variables:get_data()
   if not self._sv.players then
      self._sv.players = {}
   end
end

function JobService:add_player(player_id)
   local player_id = _get_player_id(player_id)

   local player = self._sv.players[player_id]
   if not player then
      player = radiant.create_controller('stonehearth:player_jobs_controller', player_id)
      self._sv.players[player_id] = player
      self.__saved_variables:mark_changed()
   end
   return player
end

function JobService:get_job_info(player_id, job_id)
   local player = self._sv.players[player_id]
   if not player then
      return
   end
   return player:get_job(job_id)
end


return JobService
