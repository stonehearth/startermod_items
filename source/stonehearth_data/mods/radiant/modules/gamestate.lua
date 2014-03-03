local clock_

local gamestate = {}

function gamestate.now()
   if not clock_ then
      if not radiant._root_entity then
         -- must be on the client.  this is so gross!
         radiant._root_entity = _radiant.client.get_object(1)
      end
      if not radiant._root_entity then
         return 0
      end
      clock_ = radiant._root_entity:get_component('clock')
   end
   return clock_:get_time()
end

function gamestate.is_running()
   return gamestate.now() > 0
end

return gamestate
