local Entity = _radiant.om.Entity
local rng = _radiant.csg.get_default_rng()

local AdmireFireAdjacent = class()
AdmireFireAdjacent.name = 'admire fire adjacent'
AdmireFireAdjacent.args = {
   seat = Entity,      -- the firepit seat entity
}
AdmireFireAdjacent.does = 'stonehearth:admire_fire_adjacent'
AdmireFireAdjacent.version = 2
AdmireFireAdjacent.priority = 3


function AdmireFireAdjacent:run(ai, entity, args)
   local seat = args.seat   
   self._ai = ai
   local firepit = self:_get_firepit_from_seat(seat)
   
   self:_trace_seat(ai, seat)

   -- xxx: as currently written, there's no way to add admire firepit actions
   -- without opening this file.  we shoulid use the ai-election system instead. -- tony
   radiant.entities.turn_to_face(entity, firepit:get_entity())
   while firepit:is_lit() do
      
      local random_action = rng:get_int(1, 100)
      if random_action < 30 then
         ai:execute('stonehearth:idle:breathe')
      elseif random_action > 80 and radiant.entities.get_posture(entity) ~= 'sitting' then
         radiant.entities.set_posture(entity, 'sitting')
         ai:execute('stonehearth:run_effect', { effect = 'sit_on_ground' })
      elseif radiant.entities.get_posture(entity) ~= 'sitting' then
         ai:execute('stonehearth:run_effect', { effect = 'idle_warm_hands' })
      end

      --For testing purposes, fire an event letting people know we're admiring the fire
      radiant.events.trigger_async(entity, 'stonehearth:admiring_fire', {})

   end
end

function AdmireFireAdjacent:_get_firepit_from_seat(seat)
   local center_of_attention_spot_component = seat:get_component('stonehearth:center_of_attention_spot')
   if center_of_attention_spot_component then
      local center_of_attention = center_of_attention_spot_component:get_center_of_attention()
      if center_of_attention then
         local firepit_component = center_of_attention:get_component('stonehearth:firepit')
         if firepit_component then
            return firepit_component
         end
      end
   end
   self._ai:abort('could not find firepit from seat')
end

function AdmireFireAdjacent:stop(ai, entity)
   radiant.entities.unset_posture(entity, 'sitting')
   if self._seat_trace then
      self._seat_trace:destroy()
      self._seat_trace = nil
   end
end

function AdmireFireAdjacent:_trace_seat(ai, seat)
   self._seat_trace = seat:trace_object('carry destroy trace')
      :on_destroyed(function()
         ai:abort('firepit seat was destroyed')
      end)

end

return AdmireFireAdjacent
