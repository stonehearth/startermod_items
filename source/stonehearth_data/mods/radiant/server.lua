_host:require 'radiant.lib.env' -- there's no module path installed, so use the script version...
decoda_name = "radiant server"

radiant = {
   _root_entity = _radiant.sim.create_empty_entity()
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
radiant.music = require 'modules.bgm_manager'
radiant.pathfinder = require 'modules.pathfinder'
radiant.combat = require 'lib.combat'

radiant.gamestate._start()
radiant.log.info('radiant api initialized.')

local api = {}

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
