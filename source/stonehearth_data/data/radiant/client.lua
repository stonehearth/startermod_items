local api = {}

require 'radiant.lib.env'
decoda_name = "radiant client"

radiant = {
}

radiant.log = require 'modules.log'
radiant.util = require 'lib.util'
radiant.json = require 'lualibs.dkjson'
radiant.check = require 'lib.check'
radiant.events = require 'modules.events'
radiant.mods = require 'modules.mods'
radiant.entities = require 'modules.client_entities'

return api
