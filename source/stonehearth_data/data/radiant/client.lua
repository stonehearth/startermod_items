local api = {}

require 'radiant.lib.env'
decoda_name = "radiant client"

radiant = {
}

radiant.log = require 'radiant.modules.log'
radiant.util = require 'radiant.lib.util'
radiant.json = require 'radiant.lualibs.dkjson'
radiant.check = require 'radiant.lib.check'
radiant.events = require 'radiant.modules.events'

return api
