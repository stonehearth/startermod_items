local Point3 = _radiant.csg.Point3

local SnaredBuffController = class()

function SnaredBuffController:on_buff_added(entity, buff)
   -- crate the drop
   self._trap = radiant.entities.create_entity('stonehearth:trapper:snare_trap')

   -- lease the trap to the trapper, so we know which trapper should go harvest the trap later
   --self._trap:add_component('stonehearth:lease')
   --          :acquire('trapper_reservation', entity) 

   -- put the trap in the world, at the location of the victim
   local pos = radiant.entities.get_location_aligned(entity)
   radiant.terrain.place_entity(self._trap, pos)

   -- put the victim in the trap   
   radiant.entities.move_to(entity, Point3(0, 0, 0))
   radiant.entities.add_child(self._trap, entity)
end

function SnaredBuffController:on_buff_removed(entity, buff)
   -- remove the victim from the trap and put him back in the world
   local trap_location = radiant.entities.get_location_aligned(self._trap)

   radiant.entities.remove_child(self._trap, entity)
   radiant.terrain.place_entity(entity, trap_location)
   
   radiant.entities.destroy_entity(self._trap)
end

function SnaredBuffController:get_trap()
   return self._trap
end

return SnaredBuffController
