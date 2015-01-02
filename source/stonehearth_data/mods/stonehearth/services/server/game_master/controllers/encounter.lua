local Node = require 'services.server.game_master.controllers.node'

local Encounter = class()
mixin_class(Encounter, Node)

function Encounter:initialize(info)
   self._sv.info = info
end

function Encounter:restore()
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
	assert(self._sv.info.out_edge)
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
   self._sv.script = radiant.create_controller(ename)
   self._sv.script:start(ctx, einfo)
end

return Encounter
