local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local SnareTrap = class()

SnareTrap.name = 'snare trap'
SnareTrap.does = 'stonehearth:unit_control:abilities:snare_trap'
SnareTrap.args = {
   location = Point3      -- where to move
}

SnareTrap.version = 2
SnareTrap.priority = stonehearth.constants.priorities.unit_control.CAST_SPELL

function SnareTrap:run(ai, entity, args) 
   self._entity = entity
   local location = args.location
   local radius = 4
   
   local cube = Cube3(Point3(location.x - radius,   location.y - radius,   location.z - radius),
                      Point3(location.x + radius+1, location.y + radius+1, location.z + radius+1))

   local targets = radiant.terrain.get_entities_in_cube(cube)
   
   for _, target in pairs(targets) do
      if radiant.entities.is_hostile(target, entity) then
         self:_trap_entity(target)
      end
   end

end

function SnareTrap:_trap_entity(entity)
   local buff = radiant.entities.add_buff(entity, 'stonehearth:buffs:snared')
   local trap = buff:get_controller():get_trap()

   local commands = trap:add_component('stonehearth:commands')

   commands:modify_command('harvest_trapped_beast',
      function (command_data)
         command_data.args = {
            self._entity,
            trap
         }
      end
   )

   commands:modify_command('tame_trapped_beast',
      function (command_data)
         command_data.args = {
            self._entity,
            trap
         }
      end
   )
end

return SnareTrap