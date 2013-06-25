local index = radiant.resources.load_json('/stonehearth_human_race/race_info.json')

local api = {}

function api.create_entity()
   if index then
      local gender = index.males
      local kind = gender[math.random(radiant.resources.len(gender))]
      return radiant.entities.create_entity(kind)
   end
end

return api