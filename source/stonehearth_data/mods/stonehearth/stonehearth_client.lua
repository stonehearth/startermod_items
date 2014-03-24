
stonehearth = {}

local service_creation_order = {
   'camera',
   'sky_renderer',
   'unit_control',
}

local function create_service(name)
   local path = string.format('services.client.%s.%s_service', name, name)
   local service = require(path)()

   local saved_variables = stonehearth._sv[name]
   if not saved_variables then
      saved_variables = radiant.create_datastore()
      stonehearth._sv[name] = saved_variables
   end
   service.__saved_variables = saved_variables
   service:initialize()
   stonehearth[name] = service
end

radiant.events.listen(stonehearth, 'radiant:init', function()
      stonehearth._sv = stonehearth.__saved_variables:get_data()
      for _, name in ipairs(service_creation_order) do
         create_service(name)
      end
   end)

return stonehearth
