_host:require 'radiant.lib.env' -- there's no module path installed, so use the script version...
decoda_name = "radiant client"

radiant = {
   is_server = false,
}

require 'modules.common'

function radiant.get_object(addr)
   return _radiant.client.get_object(addr)
end

function radiant.create_datastore(data)
   local datastore = _radiant.client.create_datastore()
   if data then
      datastore:set_data(data)
   end
   return datastore
end

radiant.lib = {
   Destructor = require 'modules.destructor'
}
radiant.log = require 'modules.log'
radiant.util = require 'lib.util'
radiant.resources = require 'modules.resources'
radiant.check = require 'lib.check'
radiant.math = require 'lib.math'
radiant.events = require 'modules.events'
radiant.mods = require 'modules.mods'
radiant.gamestate = require 'modules.gamestate'
radiant.entities = require 'modules.client_entities'
radiant.terrain = require 'modules.terrain'

require 'modules.timer'

-- xxx: radiant client and server need to share much more stuff!!! -- tony
function radiant.update()
   radiant.events._update()
end


local CONTROLLERS = {
   'time_tracker'
}

radiant.events.listen(radiant, 'radiant:init', function(args)
      radiant._authoring_root_entity = _radiant.client.get_authoring_root_entity()

      radiant._sv = radiant.__saved_variables:get_data()
      for _, name in ipairs(CONTROLLERS) do
         if not radiant._sv[name] then
            radiant._sv[name] = radiant.create_controller('radiant:controllers:' .. name)
         end
      end
      radiant.__saved_variables:mark_changed()

      radiant.events.create_listeners()
   end)

return radiant
