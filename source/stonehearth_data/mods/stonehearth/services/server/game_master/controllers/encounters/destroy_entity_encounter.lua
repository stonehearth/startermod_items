local DestroyEntityEncounter = class()

function DestroyEntityEncounter:activate()
   self._log = radiant.log.create_logger('game_master.encounters.destroy_entity')
end

function DestroyEntityEncounter:start(ctx, info)
   assert(info.target_entities)

   for i, entity_name in ipairs(info.target_entities) do 
      local entity_object = ctx[entity_name]
      if entity_object then
         if info.effect then
            radiant.effects.run_effect(entity_object, info.effect)
         end
         radiant.entities.kill_entity(entity_object)
      end
   end

   ctx.arc:trigger_next_encounter(ctx)
end

function DestroyEntityEncounter:stop()
end

return DestroyEntityEncounter