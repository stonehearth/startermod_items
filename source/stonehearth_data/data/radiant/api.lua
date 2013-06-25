local api = {}

native:log('loading radiant script!')
require 'radiant.lib.env'

radiant = {
   _root_entity = native:create_entity()
}

radiant.log = require 'radiant.modules.log'
radiant.util = require 'radiant.lib.util'
radiant.check = require 'radiant.lib.check'
radiant.json = require 'radiant.lualibs.dkjson'
radiant.gamestate = require 'radiant.modules.gamestate'
radiant.resources = require 'radiant.modules.resources'
radiant.events = require 'radiant.modules.events'
radiant.ai = require 'radiant.modules.ai'
radiant.effects = require 'radiant.modules.effects'
radiant.entities = require 'radiant.modules.entities'
radiant.components = require 'radiant.modules.components'
radiant.terrain = require 'radiant.modules.terrain'
radiant.mods = require 'radiant.modules.mods'

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
