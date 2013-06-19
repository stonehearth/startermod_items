local gamestate = {}
local singleton = {}

function gamestate.__init()
   singleton._clock = radiant._root_entity:add_component('clock')
end

function gamestate._start()
   singleton._initialized = true
end

function gamestate._increment_clock(interval)
   local now = gamestate.now() + interval
   singleton._clock:set_time(now)
end

function gamestate.now()
   return singleton._clock:get_time()
end

function gamestate.is_running()
   return gamestate.now() > 0
end

function gamestate.is_initialized()
   return singleton._initialized
end

gamestate.__init()
return gamestate
