local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local DropCarryingInCrate = class()
DropCarryingInCrate.name = 'drop carrying in crate'
DropCarryingInCrate.does = 'stonehearth:drop_carrying_in_crate'
DropCarryingInCrate.args = {
   crate = Entity,
}
DropCarryingInCrate.version = 2
DropCarryingInCrate.priority = 1

function DropCarryingInCrate:start(ai, entity, args)
	self._crate_location_trace = radiant.entities.trace_location(args.crate, 'crate location trace')
		:on_changed(function()
				ai:abort('drop carrying in crate destination moved.')
			end)
end


function DropCarryingInCrate:stop(ai, entity, args)
	if self._crate_location_trace then
		self._crate_location_trace:destroy()
		self._crate_location_trace = nil
	end
end

local ai = stonehearth.ai
return ai:create_compound_action(DropCarryingInCrate)
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.crate })
         :execute('stonehearth:reserve_backpack_space', { backpack_entity = ai.ARGS.crate })
         :execute('stonehearth:drop_carrying_in_backpack', { backpack_entity = ai.ARGS.crate })
