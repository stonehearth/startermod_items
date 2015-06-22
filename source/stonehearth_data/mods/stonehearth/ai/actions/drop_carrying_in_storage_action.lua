local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local DropCarryingInStorage = class()
DropCarryingInStorage.name = 'drop carrying in storage'
DropCarryingInStorage.does = 'stonehearth:drop_carrying_in_storage'
DropCarryingInStorage.args = {
   storage = Entity,
}
DropCarryingInStorage.version = 2
DropCarryingInStorage.priority = 1

function DropCarryingInStorage:start(ai, entity, args)
	self._location_trace = radiant.entities.trace_location(args.storage, 'storage location trace')
		:on_changed(function()
				ai:abort('drop carrying in storage destination moved.')
			end)
end


function DropCarryingInStorage:stop(ai, entity, args)
	if self._location_trace then
		self._location_trace:destroy()
		self._location_trace = nil
	end
end

local ai = stonehearth.ai
return ai:create_compound_action(DropCarryingInStorage)
         :execute('stonehearth:goto_entity', { entity = ai.ARGS.storage })
         :execute('stonehearth:reserve_storage_space', { storage = ai.ARGS.storage })
         :execute('stonehearth:drop_carrying_in_storage_adjacent', { storage = ai.ARGS.storage })
