_host:require 'radiant.lib.env' -- there's no module path installed, so use the script version...
decoda_name = "radiant client"

radiant = {
   is_server = false
}

function radiant.get_object(addr)
   return _radiant.client.get_object(addr)
end

radiant.log = require 'modules.log'
radiant.util = require 'lib.util'
radiant.json = require 'lualibs.dkjson'
radiant.resources = require 'modules.resources'
radiant.check = require 'lib.check'
radiant.events = require 'modules.events'
radiant.mods = require 'modules.mods'
radiant.entities = require 'modules.client_entities'

local api = {}

return api
