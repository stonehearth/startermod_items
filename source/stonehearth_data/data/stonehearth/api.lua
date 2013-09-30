-- make sure critical libaries get loaded immediately
local api = {}

function api.get_service(name)
   -- xxx: put this in a pcall so we can return a friendly
   -- error when an invalid name is passed in
   
   local service_name = string.format('services.%s.%s_service', name, name)
   local service = require(service_name)
   return service
end

return api
