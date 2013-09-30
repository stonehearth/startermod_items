decoda_name = "radiant client"


local dkjson = require 'radiant.lualibs.dkjson'
-- load external libraries before we blow away require...

require 'radiant.lib.env'

local api = {}

radiant = {
}
radiant.log = require 'modules.log'
radiant.util = require 'lib.util'
radiant.json = dkjson
radiant.resources = require 'modules.resources'
radiant.check = require 'lib.check'
radiant.events = require 'modules.events'
radiant.mods = require 'modules.mods'
radiant.entities = require 'modules.client_entities'

return api
