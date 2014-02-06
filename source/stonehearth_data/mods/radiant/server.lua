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
local _profile = {
   write_updates_longer_than = radiant.util.get_config('profile_long_frames', 0)
}

function _start_profiling(profile_this_frame)
   _profile.profile_this_frame = profile_this_frame
   if _profile.write_updates_longer_than then
      _profile.start_time = _host:get_realtime()
   end
   if _profile.profile_this_frame or _profile.write_updates_longer_than > 0 then
      ProFi:reset()
      ProFi:start()
      _profile.running = true
   end
end

function _stop_profiling()
   if _profile.running then
      ProFi:stop()
      _profile.running = false
      
      local duration      
      if _profile.start_time then
         duration = _host:get_realtime() - _profile.start_time
      end
      if _profile.profile_this_frame or (duration and duration > _profile.write_updates_longer_than) then
         local filename = string.format('lua_profile_%s.txt', radiant.gamestate.now())
         ProFi:writeReport(filename)
         if duration then
            radiant.log.write('radiant', 0, 'wrote lua profile for %s ms update to %s', duration, filename)
         end
      end
   end
end

function api.update(profile_this_frame)
   _start_profiling(profile_this_frame)

   radiant.log.spam('radiant', 'starting frame %d', radiant.gamestate.now())
   radiant.events._update()
   radiant._fire_timers()
   radiant.log.spam('radiant', 'finishing frame %d', radiant.gamestate.now())

   _stop_profiling()
   return radiant.gamestate.now()
end

return api
