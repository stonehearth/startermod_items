-- make sure critical libaries get loaded immediately
require 'lib.calendar.timekeeper'

local api = {}

function api.create_new_citizen()
   local index = radiant.resources.load_json('/stonehearth/data/human_race_info.json')
   if index then
      local gender = index.males
      local kind = gender[math.random(#gender)]
      return radiant.entities.create_entity(kind)
   end
end

return api
