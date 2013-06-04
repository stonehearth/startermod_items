local api = {}

function api._promote_ai(entity, ai)
   if ai then
      if ai.actions then
         if ai.actions then
            for _, uri in radiant.resources.pairs(ai.actions) do
               radiant.ai.add_action(entity, uri)
            end
         end
      end
   end
end

function api._promote_outfits(entity, outfits)
   for _, outfit in radiant.resources.pairs(outfits) do
      radiant.entities.add_outfit(entity, outfit)
   end   
end

--pass in the dude to promote and the uri of his class_info
function api.promote(entity, uri)
   local class_info = radiant.resources.load_json(uri)
   if class_info then
      api._promote_outfit(entity, class_info.outfits)
      api._promote_ai(entity, class_info.ai)
   end
end

return api

