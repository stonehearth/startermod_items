-- make sure critical libaries get loaded immediately

function get_service(name)
   local service_name = string.format('services.%s.%s_service', name, name)
   local service = require(service_name)
   return service
end

if radiant.is_server then
   api = {
      analytics = get_service('analytics'),
      ai = get_service('ai'),
      calendar = get_service('calendar'),
      combat = get_service('combat'),
      event = get_service('event'),
      personality_serivce = get_service('personality'),
      inventory = get_service('inventory'),
      population = get_service('population'),
      object_tracker = get_service('object_tracker'),
      worker_scheduler = get_service('worker_scheduler'),
      world_generation = get_service('world_generation'),
      build = get_service('build'),   
      game_master = get_service('game_master'),
   }
else
   api = {
      camera = get_service('camera'),
   }
end

return api
