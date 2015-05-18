local PlayerJobsController = class()

function PlayerJobsController:initialize(player_id)
   self._sv.player_id = player_id
   if not self._sv.jobs then
      self._sv.jobs = {}

      local index = radiant.resources.load_json('stonehearth:jobs:index')
      if index and index.jobs then
         for id, info in pairs(index.jobs) do
            self._sv.jobs[id] = radiant.create_controller('stonehearth:job_info_controller', info)
         end
      end
      self.__saved_variables:mark_changed()
   end
end

function PlayerJobsController:get_job(id)
   return self._sv.jobs[id]
end

return PlayerJobsController
