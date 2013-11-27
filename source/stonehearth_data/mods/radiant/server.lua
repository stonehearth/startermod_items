_host:require 'radiant.lib.env' -- there's no module path installed, so use the script version...
decoda_name = "radiant server"

radiant = {
   is_server = true,
   _root_entity = _radiant.sim.create_empty_entity()
}
-- The root entity should not be interpolated; if it is, all children (the entire universe)
-- will be dirtied, and horde will walk the scene-graph _every_ _single_ _frame_.
radiant._root_entity:add_component('mob'):set_interpolate_movement(false)

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
radiant.log.info('radiant api initialized.')

local api = {}

function api.update(interval)
   radiant.gamestate._increment_clock(interval)
   radiant.events._update()
   return radiant.gamestate.now()
end

return api
