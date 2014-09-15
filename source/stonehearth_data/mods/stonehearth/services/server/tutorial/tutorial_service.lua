TutorialService = class()

function TutorialService:initialize()
   self._sv = self.__saved_variables:get_data()
end

-- ug!  this should ideally be on the client.
-- That can't happen until we standarize the way we make sevices of
-- mods (ideally as an robject) and make radiant by into that too so we can call
-- (radiant:get_client_service, radiant.terrain) from javascript.  UG! -- tony
--
function TutorialService:get_first_wood_harvest(session, response)
   local world_entity_trace
   world_entity_trace = radiant.terrain.trace_world_entities('terrain service', 
      function (id, entity)
         local entity_uri = entity:get_uri()
         if radiant.entities.is_material(entity, 'wood resource') then
            response:resolve({ entity = entity })
            world_entity_trace:destroy()
         end
      end
   )   
end

return TutorialService

