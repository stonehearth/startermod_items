local api = {}

---[[
--pass in the dude to promote and the uri of his class_info
function api.promote(entity, uri)
   local class_info = radiant.resources.load_json(uri)

   if class_info then
      for _, outfit in radiant.resources.pairs(class_info.outfits) do
         radiant.entities.add_outfit(entity, outfit)
      end
   end
end
--]]

return api
