local api = {}

_host:log('loading server radiant script')

local dkjson = require 'radiant.lualibs.dkjson'
-- load external libraries before we blow away require...

require 'radiant.lib.env'

decoda_name = "radiant server"

radiant = {
   _root_entity = _radiant.sim.create_empty_entity()
}

radiant.log = require 'modules.log'
radiant.util = require 'lib.util'
radiant.check = require 'lib.check'
radiant.json = dkjson
radiant.gamestate = require 'modules.gamestate'
radiant.resources = require 'modules.resources'
radiant.events = require 'modules.events'
radiant.ai = require 'modules.ai'
radiant.effects = require 'modules.effects'
radiant.entities = require 'modules.entities'
radiant.terrain = require 'modules.terrain'
radiant.mods = require 'modules.mods'
radiant.music = require 'modules.bgm_manager'
radiant.pathfinder = require 'modules.pathfinder'

radiant.gamestate._start()
radiant.log.info('radiant api initialized.')

function api.update(interval)
   radiant.gamestate._increment_clock(interval)
   radiant.events._update()
   --[[
   if now > self._lastProfileReport + 5000 then
      ProFi:stop();
      ProFi:writeReport();
      ProFi:reset();
      ProFi:start();
      self._lastProfileReport = now
   end
   copas.step(0)
   ]]
   return radiant.gamestate.now()
end

function api.call_game_hook(stage)
   radiant.events._call_game_hook(stage)
end

return api
