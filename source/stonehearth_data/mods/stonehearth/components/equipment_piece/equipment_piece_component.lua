local EquipmentPieceComponent = class()

function EquipmentPieceComponent:__init(entity)
   self._entity = entity
   self._info = {}
end

function EquipmentPieceComponent:extend(json)
   assert(json.render_type)
   self._info = json
end


function EquipmentPieceComponent:get_info()
   return self._info
end

return EquipmentPieceComponent