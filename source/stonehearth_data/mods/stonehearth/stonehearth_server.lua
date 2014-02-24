
stonehearth = {
   constants = require 'constants'
}

local function create_datastore(stonehearth_datastore, name)
   local data = stonehearth_datastore:get_data()
   local datastore = data[name]
   if not datastore then
      datastore = _radiant.sim.create_data_store()
      data[name] = datastore
      stonehearth_datastore:mark_changed()
   end
   return datastore
end

local function create_service(stonehearth_datastore, name)
   local path = string.format('services.%s.%s_service', name, name)         
   local datastore = create_datastore(stonehearth_datastore, name)
   return require(path)(datastore)
end

radiant.events.listen(stonehearth, 'radiant:construct', function(args)
      
      local datastore = args.datastore
      local service_creation_order = {
         'ai',
         'events',
         'calendar',
         'combat',
         'substitution',
         'personality',
         'inventory',
         'scenario',
         'population',
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
      for _, name in ipairs(service_creation_order) do
         stonehearth[name] = create_service(datastore, name)
      end
   end)
return stonehearth
