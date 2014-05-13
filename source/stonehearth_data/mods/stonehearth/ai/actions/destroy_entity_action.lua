local log = radiant.log.create_logger('death')

local DestroyEntity = class()

DestroyEntity.name = 'destroy entity'
DestroyEntity.does = 'stonehearth:destroy_entity'
DestroyEntity.args = {}
DestroyEntity.version = 2
DestroyEntity.priority = 1

local destroy_scheduled = {}

function DestroyEntity:run(ai, entity, args)
   if entity == nil or not entity:is_valid() then
      -- how did we get here? ai should never have called this
      log:error('DestroyEntity called with invalid entity')
      assert(false)
   end

   if destroy_scheduled[entity:get_id()] ~= nil then
      -- TODO: figure out what the AI should do after someone dies
      -- for now, don't infinite loop and don't allow lower priority AI to run
      log:detail('%s in DestroyEntity:run() waiting to be destroyed. Suspending ai.', entity)
      ai:suspend()
      return
   end

   log:info('%s is dying', entity)
end

function DestroyEntity:stop(ai, entity, args)
   log:detail('%s in DestroyEntity:stop()', entity)

   destroy_scheduled[entity:get_id()] = entity

   -- Shouldn't destroy the entity while the AI is running
   stonehearth.calendar:set_timer(1,
      function ()
         if entity:is_valid() then
            log:detail('Destroying %s', entity)
            local id = entity:get_id()
            radiant.entities.destroy_entity(entity)
            destroy_scheduled[id] = nil
         end
      end
   )
end

return DestroyEntity
