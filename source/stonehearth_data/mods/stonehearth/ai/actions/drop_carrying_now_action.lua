-- @title stonehearth:drop_carrying_now 
-- @book reference
-- @section activities

local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f

--[[ @markdown
Use stonehearth:drop\_carrying\_now when an entity should drop whatever they are carrying immediately. 
Often, this is used before starting an action that requires the entity's hands to be empty. 

For example,  _Admire\_fire_, _death\_action_, _harvest\_plant_, and _combat\_action_ all start by calling: 
      ai:execute('stonehearth:drop\_carrying\_now')

You can call this action even when you are unsure whether the entity is carrying anything; in fact, use it as the first action in a sequence to make sure that the entity will go into subsequent states with nothing in their arms
If the drop animation is interrupted, the item will still get dropped. 

stonehearth:drop\_carrying\_now is only implemented by the drop\_carrying\_now\_action.lua file: 
]]--

local DropCarryingNow = class()
DropCarryingNow.name = 'drop carrying now'
DropCarryingNow.does = 'stonehearth:drop_carrying_now'
DropCarryingNow.args = {
}
DropCarryingNow.version = 2
DropCarryingNow.priority = 1

-- Sets current carrying to nil and sets think output
function DropCarryingNow:start_thinking(ai, entity, args)
   ai.CURRENT.carrying = nil
   ai:set_think_output()
end


-- Drops whatever we are carrying. 
-- First, check if we are carrying anything. If so, drop that item in front of us. If that
-- space is empty, or under where we are currently standing if not
function DropCarryingNow:run(ai, entity, args)
   self._curr_carrying = radiant.entities.get_carrying(entity)
   if not self._curr_carrying then
      return
   end

   local mob = entity:add_component('mob')
   local facing = mob:get_facing()
   local location = mob:get_world_grid_location()

   local front = self:_get_front_grid_location(location, facing)

   if _physics:is_standable(front) then
      self._drop_location = front
   else
      -- drop at our feet is front is blocked
      self._drop_location = location
   end

   radiant.entities.turn_to_face(entity, front)
   ai:execute('stonehearth:run_effect', { effect = 'carry_putdown' })

   -- place the item now, don't wait for the compound action (if it exists) to terminate
   self:stop(ai, entity, args)
end


-- On stop, if the animation is interrupted, complete the drop anyway since we've already let go of the item!
function DropCarryingNow:stop(ai, entity, args)
   if self._drop_location then
      radiant.entities.drop_carrying_on_ground(entity, self._drop_location)
      self._drop_location = nil
   end
end

-- Find the grint in front of us and return it
function DropCarryingNow:_get_front_grid_location(location, facing)
   -- align to one of the four non-diagonal adjacent voxels
   facing = radiant.math.round(facing / 90) * 90

   -- 0 degress is along the negative z axis
   local offset = radiant.math.rotate_about_y_axis(-Point3f.unit_z, facing):to_closest_int()
   local front = location + offset
   return front
end


return DropCarryingNow
