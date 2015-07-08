local DropCarryingWhenIdle = class()

DropCarryingWhenIdle.name = 'drop carrying when idle'
DropCarryingWhenIdle.does = 'stonehearth:idle:bored'
DropCarryingWhenIdle.args = {
	hold_position = 'boolean'
}
DropCarryingWhenIdle.version = 2
DropCarryingWhenIdle.priority = 10

function DropCarryingWhenIdle:start_thinking(ai, entity, args)
	if args.hold_position then
		return
	end

	local backpack = entity:get_component('stonehearth:storage')
	local items_in_backpack = backpack and not backpack:is_empty()

	if ai.CURRENT.carrying or items_in_backpack then
	   ai:set_think_output()
	end
end

local function everything()
   return true
end

function DropCarryingWhenIdle:run(ai, entity, args)
	local carrying = radiant.entities.get_carrying(entity)
	local backpack = entity:get_component('stonehearth:storage')
   local radius = 3
   
	while carrying do
		ai:execute('stonehearth:wander_within_leash', { radius = radius })
		ai:execute('stonehearth:drop_carrying_now')
		if backpack and not backpack:is_empty() then
	   	ai:execute('stonehearth:pickup_item_type_from_backpack', { filter_fn = everything, description = 'dropping carrying'})
	   end
	   carrying = radiant.entities.get_carrying(entity)
	   radius = 2
	end
end

return DropCarryingWhenIdle
