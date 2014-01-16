local PerfTestCallHandler = class()

function PerfTestCallHandler:get_world_generation_done(session, response)
  local wgs = radiant.mods.load('stonehearth').world_generation
  local wg = wgs:create_world(true, 1337)

  radiant.events.listen(radiant.events, 'stonehearth:generate_world_progress', self, function(s, e)
    if e.progress and e.progress >= 100 then
      response:resolve(true)
    end
  end)
end

return PerfTestCallHandler
