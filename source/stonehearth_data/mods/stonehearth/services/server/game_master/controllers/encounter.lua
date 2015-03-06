local Node = require 'services.server.game_master.controllers.node'

local Encounter = class()
mixin_class(Encounter, Node)

function Encounter:initialize(info)
   self._log = radiant.log.create_logger('game_master.encounter')
   self._sv._info = info

   local etype, einfo = self:_get_script_info()
   assert(etype and einfo, string.format('missing encounter data for type "%s"', etype))

   local ename = 'stonehearth:game_master:encounters:' .. etype   
   local script = radiant.create_controller(ename, einfo)
   if not script then
      self._log:error('could not create controller for encounter type "%s".  bailing.', etype)
      return
   end
   self._sv.script = script
   self.__saved_variables:mark_changed()
end

function Encounter:restore()
end

function Encounter:destroy()
   --self._sv.script:destroy()
end

-- get the name of the edge which leads to this encounter.
--
function Encounter:get_in_edge()
	assert(self._sv._info.in_edge)
   return self._sv._info.in_edge
end

-- get the name of the edge which this encounter leads to.
--
function Encounter:get_out_edge()	
	local script = self._sv.script

	-- if the subtype script provides a custom implementation, use that
	if script.get_out_edge then
      local etype, einfo = self:_get_script_info()
		return script:get_out_edge(einfo)
	end

	-- return what's in the json file.
   return self._sv._info.out_edge
end


-- should the node spawn now?
function Encounter:can_start(ctx)
   if self._sv.script.can_start then
      local etype, einfo = self:_get_script_info()
      return self._sv.script:can_start(ctx, einfo)
   end
   return true
end

-- start the encounter.
--
function Encounter:start(ctx)
   local etype, einfo = self:_get_script_info()
   self._sv.script:start(ctx, einfo)
end

function Encounter:stop()
   assert(self._sv.script)
   assert(self._sv.script.stop)
   self._sv.script:stop()
end

function Encounter:_get_script_info()
   local info = self._sv._info
   local etype = info.encounter_type
   local einfo = info[etype .. '_info']
   return etype, einfo
end

return Encounter
