_host:require 'radiant.lib.env' -- there's no module path installed, so use the script version...
decoda_name = "radiant server"

radiant = {
   is_server = true,
   _root_entity = _radiant.sim.get_entity(addr)
}
require 'modules.common'

function radiant.get_entity(addr)
   return _radiant.sim.get_entity(addr)
end

function radiant.create_datastore(data)
   local datastore = _radiant.sim.create_datastore()
   if data then
      datastore:set_data(data)
   end
   return datastore
end

function radiant.exit(code)
   _host:exit(code)
end

radiant.lib = {
   Destructor = require 'modules.destructor'
}
radiant.log = require 'modules.log'
radiant.util = require 'lib.util'
radiant.check = require 'lib.check'
radiant.math = require 'lib.math'
radiant.gamestate = require 'modules.gamestate'
radiant.resources = require 'modules.resources'
radiant.events = require 'modules.events'
radiant.effects = require 'modules.effects'
radiant.entities = require 'modules.entities'
radiant.terrain = require 'modules.terrain'
radiant.mods = require 'modules.mods'

radiant.log.info('radiant', 'radiant api initialized.')

require 'modules.timer'

function radiant.not_reached(reason, ...)
   local reason = reason and string.format(reason, ...) or 'no reason given'
   assert(false, 'executed theoretically unreachable code: ' .. reason)
end

local ProFi = require 'lib.ProFi'
local _profile = {
   enabled = radiant.util.get_config('enable_lua_profling', false),
   write_updates_longer_than = radiant.util.get_config('profile_long_frames', 0) / 1000.0,
   count = 1
}

--[[
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
         local filename = string.format('lua_profile_%s__%d_ms.txt', radiant.gamestate.now(), duration * 1000)
         ProFi:writeReport(filename)
         if duration then
            radiant.log.write('radiant', 0, 'wrote lua profile for %.3f ms update to %s', duration, filename)
         end
      end
   end
end

]]

function _start_profiling(dump_profile)
   if dump_profile then
      ProFi:stop()
      _profile.running = false

      local filename = string.format('lua_profile_%02d__%d.txt', _profile.count, radiant.gamestate.now())
      ProFi:writeReport(filename)
      radiant.log.write('radiant', 0, 'wrote lua profile %s', filename)
      _profile.count = _profile.count + 1
   end
   if not _profile.running then
      ProFi:start()
      _profile.running = true
   end
end

function _stop_profiling()
   --ProFi:stop()
end

function radiant.update(profile_this_frame)
   if _profile.enabled then
      _start_profiling(profile_this_frame)
   end

   radiant.log.spam('radiant', 'starting frame %d', radiant.gamestate.now())
   radiant.events._update()
   radiant.log.spam('radiant', 'finishing frame %d', radiant.gamestate.now())

   if _profile.enabled then
      _stop_profiling()
   end
end

local CONTROLLERS = {
   'time_tracker'
}

radiant.events.listen(radiant, 'radiant:init', function(args)
      radiant._root_entity = _radiant.sim.get_entity(1)

      radiant._sv = radiant.__saved_variables:get_data()
      for _, name in ipairs(CONTROLLERS) do
         if not radiant._sv[name] then
            radiant._sv[name] = radiant.create_controller('radiant:controllers:' .. name)
         end
      end
      radiant.__saved_variables:mark_changed()

      radiant.events.create_listeners()
   end)

return radiant
