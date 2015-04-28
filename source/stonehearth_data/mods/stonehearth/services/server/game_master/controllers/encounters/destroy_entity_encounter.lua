local Entity = _radiant.om.Entity

local DestroyEntityEncounter = class()

-- Deletes a set of objects described in an array
-- If an object in the array is itself an array of entities,
-- delete all the entities in that array

--If on restore, there are leftover guys that we haven't actually managed
--to delete yet, delete them, 
function DestroyEntityEncounter:restore()
   radiant.events.listen_once(radiant, 'radiant:game_loaded', function(e)
      if self._sv.ctx and self._sv.info then
         self:_delete_everything(self._sv.ctx, self._sv.info)
      end
   end)
end

function DestroyEntityEncounter:activate()
   self._log = radiant.log.create_logger('game_master.encounters.destroy_entity')
end

--On load, if there are any entities still around, because we saved/loaded
--before the effect could finish, 

--Given a bunch of paths, destroy them
function DestroyEntityEncounter:start(ctx, info)
   assert(info.target_entities)
   self._sv.num_destroyed = 0
   self._sv.ctx = ctx
   self._sv.info = info

   self:_delete_everything(ctx, info)


   --If we are always supposed to continue, or if 
   --we successfully destroyed one of the things, then trigger next encounter
   if info.continue_always or self._sv.num_destroyed > 0 then
      ctx.arc:trigger_next_encounter(ctx)
   end
end

--Iterate through everything in the to-be-deleted list and delete it
function DestroyEntityEncounter:_delete_everything(ctx, info)
   for i, entity_name in ipairs(info.target_entities) do 
      local delete_target = ctx:get(entity_name)
      self:_recursive_delete(delete_target, info.effect)
   end
end

function DestroyEntityEncounter:_recursive_delete(delete_target, effect)
   if delete_target then
      if radiant.util.is_a(delete_target, Entity) then
         return self:_destroy_entity_with_effect(delete_target, effect)
      elseif type(delete_target) == 'table' then
         for key, value in pairs(delete_target) do
            self:_recursive_delete(value, effect)
         end
      end
   end
end

-- Destroy the entity in question, if it's valid
-- If we specified an effect, run that effect first. 
-- TODO: make fire effect dissolve better
-- TODO: make undead disappear slowly, one after another
function DestroyEntityEncounter:_destroy_entity_with_effect(entity, effect)
   if entity:is_valid() then
      if effect then
         local effect = radiant.effects.run_effect(entity, '/stonehearth/data/effects/fire_effect') --'/stonehearth/data/effects/firepit_effect') --effect)
         effect:set_finished_cb(function()
            radiant.entities.destroy_entity(entity)
            effect:set_finished_cb(nil)
            effect = nil
         end)
      else
         radiant.entities.destroy_entity(entity)
      end
      self._sv.num_destroyed = self._sv.num_destroyed + 1
   end
end

function DestroyEntityEncounter:stop()
end

return DestroyEntityEncounter