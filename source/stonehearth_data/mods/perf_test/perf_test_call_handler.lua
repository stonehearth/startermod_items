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
      local seed = 1337
      wgs:initialize(seed, true)

      local blueprint = wgs.blueprint_generator:generate_blueprint(5, 5, seed)
      wgs:set_blueprint(blueprint)
      wgs:generate_all_tiles()

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
