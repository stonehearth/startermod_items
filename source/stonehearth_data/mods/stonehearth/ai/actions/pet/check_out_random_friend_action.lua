local PetFns = require 'ai.actions.pet.pet_fns'
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local CheckOutRandomFriend = class()

CheckOutRandomFriend.name = 'check out random friend'
CheckOutRandomFriend.does = 'stonehearth:pet:play'
CheckOutRandomFriend.args = {}
CheckOutRandomFriend.version = 2
CheckOutRandomFriend.priority = 1
CheckOutRandomFriend.weight = 1

function CheckOutRandomFriend:__init()
   self._friend_range = radiant.util.get_config('sight_radius', 64)
end

function CheckOutRandomFriend:start_thinking(ai, entity, args)
   local friends, num_friends = PetFns.get_friends_nearby(entity, self._friend_range)
   if num_friends > 0 then
      self._friends = friends
      self._num_friends = num_friends

      -- would pass friends to run(), but that doesn't happen
      ai:set_think_output() 
   else
      local log = ai:get_log()
      log:info('no friends nearby')
   end
end

function CheckOutRandomFriend:run(ai, entity, args)
   local roll = rng:get_int(1, self._num_friends)
   local target = self._friends[roll]
   local target_location = radiant.entities.get_world_grid_location(target)

   for i=1, 8 do
      local destination = radiant.terrain.find_placement_point(target_location, 1, 4)
      ai:execute('stonehearth:go_toward_location', { destination = destination })
   end
end

return CheckOutRandomFriend
