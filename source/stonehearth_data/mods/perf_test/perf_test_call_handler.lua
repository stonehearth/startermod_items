local PerfTestCallHandler = class()

local is_running = false

function PerfTestCallHandler:__init()
end

function PerfTestCallHandler:set_is_running()
  is_running = true
end

function PerfTestCallHandler:get_world_generation_done(session, response)

  if is_running then
    local wgs = radiant.mods.load('stonehearth').world_generation
    local wg = wgs:create_world(true, 1337)

    radiant.events.listen(radiant.events, 'stonehearth:generate_world_progress', self, function(s, e)
      if e.progress and e.progress >= 100 then
        response:resolve(true)
      end
    end)
  else
    response:resolve(false)
  end
end

return PerfTestCallHandler
