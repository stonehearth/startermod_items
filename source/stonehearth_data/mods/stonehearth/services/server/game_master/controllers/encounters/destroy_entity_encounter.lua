local DestroyEntityEncounter = class()

function DestroyEntityEncounter:activate()
   self._log = radiant.log.create_logger('game_master.encounters.destroy_entity')
end

function DestroyEntityEncounter:start(ctx, info)
   assert(info.target_entities)
   local num_destroyed = 0

   for i, entity_name in ipairs(info.target_entities) do 
      local entity_object = ctx[entity_name]
      if entity_object and entity_object:is_valid() then
         if info.effect then
            radiant.effects.run_effect(entity_object, info.effect)
         end
         --
         radiant.entities.destroy_entity(entity_object)
         num_destroyed = num_destroyed + 1
      end
   end

   if info.continue_always or num_destroyed > 0 then
      ctx.arc:trigger_next_encounter(ctx)
   end
end

function DestroyEntityEncounter:stop()
end

return DestroyEntityEncounter