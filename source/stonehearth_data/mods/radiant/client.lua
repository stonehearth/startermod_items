_host:require 'radiant.lib.env' -- there's no module path installed, so use the script version...
decoda_name = "radiant client"

radiant = {
   is_server = false,
}

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

function radiant.create_controller(...)
   local args = { ... }
   local name = table.remove(args, 1)
   local datastore = radiant.create_datastore()
   local controller = datastore:create_controller('controllers', name)
   if controller and controller.initialize then
      controller:initialize(unpack(args))
   end
   return controller
end

radiant.lib = {}
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

return radiant
