function get_service(name)
   local service_name = string.format('services.%s.%s_service', name, name)
   return require(service_name)
end

stonehearth = {}
stonehearth.camera = get_service('camera')

require 'renderers.sky.sky_renderer'

return stonehearth
