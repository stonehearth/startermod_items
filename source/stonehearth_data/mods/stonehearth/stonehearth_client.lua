
stonehearth = {}

-- For services that handle input, make sure you load them in the order
-- that you want input to be handled (i.e. xz-region-selection gets priority
-- over unit-selection/orders.)
local service_creation_order = {
   'browser_data',
   'input',
   'camera',
   'sound',
   'renderer',
   'sky_renderer',
   'selection',
   'hilight',
   'build_editor',
   'subterranean_view',
   'terrain_highlight',
   'party_editor',
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
   saved_variables:set_controller(service)
   service._sv = saved_variables:get_data()
   service:initialize()
   stonehearth[name] = service
end

radiant.events.listen(stonehearth, 'radiant:init', function()
      -- global config
      radiant.terrain.set_config_file('stonehearth:terrain_block_config', false)

      stonehearth._sv = stonehearth.__saved_variables:get_data()
      for _, name in ipairs(service_creation_order) do
         create_service(name)
      end
   end)

return stonehearth
