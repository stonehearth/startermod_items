
stonehearth = {}

-- For services that handle input, make sure you load them in the order
-- that you want input to be handled (i.e. xz-region-selection gets priority
-- over unit-selection/orders.)
local service_creation_order = {
   'input',
   'camera',
   'renderer',
   'sky_renderer',
   'unit_control',
   'selection',
   'hilight'
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
