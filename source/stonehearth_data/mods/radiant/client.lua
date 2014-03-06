_host:require 'radiant.lib.env' -- there's no module path installed, so use the script version...
decoda_name = "radiant client"

radiant = {
   is_server = false,
}

function radiant.get_object(addr)
   return _radiant.client.get_object(addr)
end

radiant.log = require 'modules.log'
radiant.util = require 'lib.util'
radiant.resources = require 'modules.resources'
radiant.check = require 'lib.check'
radiant.events = require 'modules.events'
radiant.mods = require 'modules.mods'
radiant.gamestate = require 'modules.gamestate'
radiant.entities = require 'modules.client_entities'

require 'modules.timer'

local api = {}

-- xxx: radiant client and server need to share much more stuff!!! -- tony
function api.update()
   radiant.events._update()
   radiant._fire_timers()
end

return api
