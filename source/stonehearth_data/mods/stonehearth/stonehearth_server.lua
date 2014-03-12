
stonehearth = {
   constants = require 'constants'
}

local service_creation_order = {
   'ai',
   'events',
   'calendar',
   'combat',
   'substitution',
   'personality',
   'inventory',
   'population',
   'scenario',
   'object_tracker',
   'world_generation',
   'build',
   'game_master',
   'analytics',
   'tasks',
   'terrain',
   'threads',
   'town',         
}

local function create_service(name)
   local path = string.format('services.%s.%s_service', name, name)
   return require(path)()
end

local function create_services(init_fn, ...)
   for _, name in ipairs(service_creation_order) do
      local service = create_service(name)
      if service[init_fn] then
         service[init_fn](service, ...)
      end
      stonehearth[name] = service
   end
end

radiant.events.listen(stonehearth, 'radiant:construct', function(args)
      create_services('initialize')
   end)

radiant.events.listen(stonehearth, 'radiant:load', function(e)      
      create_services('restore', e.savestate)
   end)

return stonehearth
