local Node = require 'services.server.game_master.controllers.node'

local Encounter = class()
mixin_class(Encounter, Node)

function Encounter:initialize(info)
   self._sv.info = info
   self._log = radiant.log.create_logger('game_master.encounter')
end

function Encounter:restore()
end

function Encounter:destroy()
   radiant.destroy_controller(self._sv.script)
end

-- get the name of the edge which leads to this encounter.
--
function Encounter:get_in_edge()
	assert(self._sv.info.in_edge)
   return self._sv.info.in_edge
end

-- get the name of the edge which this encounter leads to.
--
function Encounter:get_out_edge()	
	local script = self._sv.script

	-- if the subtype script provides a custom implementation, use that
	if script.get_out_edge then
		return script:get_out_edge()
	end

	-- return what's in the json file.
   return self._sv.info.out_edge
end

-- start the encounter.
--
function Encounter:start(ctx)
   assert(not self._sv.script)
   local etype = self._sv.info.encounter_type
   local einfo = self._sv.info[etype .. '_info']
   local ename = 'stonehearth:game_master:encounters:' .. etype   
   
   assert(einfo)
   local script = radiant.create_controller(ename)
   if not script then
      self._log:error('could not create controller for encounter type "%s".  bailing.', etype)
      return
   end
   self._sv.script = script
   script:start(ctx, einfo)
end

function Encounter:stop()
   assert(self._sv.script)
   assert(self._sv.script.stop)
   self._sv.script:stop()
end

return Encounter
