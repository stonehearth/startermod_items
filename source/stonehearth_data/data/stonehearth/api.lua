-- make sure critical libaries get loaded immediately
local timekeeper = require 'lib.calendar.timekeeper'
local name_generator = require 'lib.factions.name_generator'
local WorkerScheduler = require 'lib.worker_scheduler.worker_scheduler'

local api = {}
local worker_schedulers = {}

function api.create_new_citizen()
   local index = radiant.resources.load_json('/stonehearth/data/human_race_info.json')
   if index then
      local gender = index.males
      local kind = gender[math.random(#gender)]
      return radiant.entities.create_entity(kind)
   end
end

api.generate_random_name = function(...)
   return name_generator.generate_random_name(...)
end

function api.get_worker_scheduler(faction)
   radiant.check.is_string(faction)
   local scheduler = worker_schedulers[faction]
   if not scheduler then
      scheduler = WorkerScheduler(faction)
      worker_schedulers[faction] = scheduler
   end
   return scheduler
end

return api
