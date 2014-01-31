_host:require 'radiant.lib.env' -- there's no module path installed, so use the script version...
decoda_name = "radiant server"

radiant = {
   is_server = true,
   _root_entity = _radiant.sim.get_entity(1)
}

radiant.log = require 'modules.log'
radiant.util = require 'lib.util'
radiant.check = require 'lib.check'
radiant.json = require 'lualibs.dkjson'
radiant.gamestate = require 'modules.gamestate'
radiant.resources = require 'modules.resources'
radiant.events = require 'modules.events'
radiant.effects = require 'modules.effects'
radiant.entities = require 'modules.entities'
radiant.terrain = require 'modules.terrain'
radiant.mods = require 'modules.mods'
radiant.pathfinder = require 'modules.pathfinder'

radiant.gamestate._start()
radiant.log.info('server', 'radiant api initialized.')

require 'modules.timer'

function radiant.not_reached(reason, ...)
   local reason = reason and string.format(reason, ...) or 'no reason given'
   assert(false, 'executed theoretically unreachable code: ' .. reason)
end

local api = {}

local ProFi = require 'lib.ProFi'
function api.update(interval, profile)
   if profile then
      ProFi:start()
   end
   radiant.gamestate._increment_clock(interval)
   radiant.events._update()
   radiant._fire_timers()
   if profile then
      ProFi:stop()
      ProFi:writeReport()
   end
   return radiant.gamestate.now()
end

return api
