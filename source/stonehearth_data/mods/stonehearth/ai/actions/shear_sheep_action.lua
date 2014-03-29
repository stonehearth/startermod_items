--[[
   Tell a worker to collect food and resources from a plant. 
]]

local Point3 = _radiant.csg.Point3

local ShearSheepAction = class()
local log = radiant.log.create_logger('actions.shear_sheep')

ShearSheepAction.name = 'shear sheep'
ShearSheepAction.does = 'stonehearth:shear_sheep'
ShearSheepAction.version = 1
ShearSheepAction.priority = 9

-- XXX, it seems like this whole thing is pretty similar to harvesting plants. Can the two be unified?
function ShearSheepAction:__init(ai, entity)
   self._ai = ai
   self._entity = entity

end

function ShearSheepAction:run(ai, entity, path)
   local sheep = path:get_destination()

   if not sheep then
      ai:abort('tried to shear a sheep, but there is no sheep!')
   end

   ai:execute('stonehearth:follow_path', path)
   radiant.entities.turn_to_face(entity, sheep)

   local factory = sheep:get_component('stonehearth:renewable_resource_node')
   if factory then
      ai:execute('stonehearth:run_effect','fiddle')
      local front_point = self._entity:get_component('mob'):get_location_in_front()
      local spawned_item = factory:spawn_resource(Point3(front_point.x, front_point.y, front_point.z))

      --Log it in the personality component
      local spawned_item_name = spawned_item:get_component('unit_info'):get_display_name()
      local personality_component = entity:get_component('stonehearth:personality')
      personality_component:add_substitution('gather_target', spawned_item_name)
      personality_component:add_substitution_by_parameter('personality_gather_reaction', personality_component:get_personality(), 'stonehearth:settler_journals:gathering_supplies')

      radiant.events.trigger(stonehearth.personality, 'stonehearth:journal_event', 
                            {entity = entity, description = 'gathering_supplies'})

   end   

end

return ShearSheepAction
