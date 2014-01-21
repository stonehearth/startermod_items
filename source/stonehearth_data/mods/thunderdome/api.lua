-- make sure critical libaries get loaded immediately

function get_service(name)
   local service_name = string.format('services.%s.%s_service', name, name)
   local service = require(service_name)
   return service
end

if radiant.is_server then
   api = {
      character_controller = get_service('character_controller'),
   }
end

return api
