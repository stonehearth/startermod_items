local log = radiant.log.create_logger('death')

local DestroyEntity = class()

DestroyEntity.name = 'destroy entity'
DestroyEntity.does = 'stonehearth:destroy_entity'
DestroyEntity.args = {}
DestroyEntity.version = 2
DestroyEntity.priority = 1

local destroy_scheduled = {}

function DestroyEntity:start_thinking(ai, entity, args)
   if not entity and not entity:is_vaild() then
      ai:set_think_output()
      return
   end
   if not destroy_scheduled[entity:get_id()]  then
      log:info('%s is dying', entity)
      ai:set_think_output()
   end
end

function DestroyEntity:run(ai, entity, args)
   log:detail('%s in DestroyEntity:run()', entity)

   if entity and entity:is_valid() then
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
end

return DestroyEntity
