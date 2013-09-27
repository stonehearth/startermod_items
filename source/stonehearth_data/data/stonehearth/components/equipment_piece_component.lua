local EquipmentPiece = class()

function EquipmentPiece:__init(entity)
   self._entity = entity
   self._info = {}
end

function EquipmentPiece:extend(json)
   assert(json.render_type)
   self._info = json
end


function EquipmentPiece:get_info()
   return self._info
end

return EquipmentPiece