_host:require 'radiant.lib.env' -- there's no module path installed, so use the script version...
decoda_name = "radiant client"

radiant = {
}

radiant.log = require 'modules.log'
radiant.util = require 'lib.util'
<<<<<<< HEAD:source/stonehearth_data/mods/radiant/client.lua
radiant.json = require 'lualibs.dkjson'
=======
radiant.json = dkjson
radiant.resources = require 'modules.resources'
>>>>>>> 732484d8076218d8d28a9e4e6dea871a89ce3bb0:source/stonehearth_data/data/radiant/client.lua
radiant.check = require 'lib.check'
radiant.events = require 'modules.events'
radiant.mods = require 'modules.mods'
radiant.entities = require 'modules.client_entities'

local api = {}

return api
