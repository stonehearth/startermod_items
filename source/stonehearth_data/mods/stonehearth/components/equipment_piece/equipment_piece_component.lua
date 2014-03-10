local EquipmentPieceComponent = class()

function EquipmentPieceComponent:__create(entity, json)
   self._entity = entity

   assert(json.render_type)
   self._info = json
end

function EquipmentPieceComponent:get_info()
   return self._info
end

function EquipmentPieceComponent:get_render_type()
   return self._info.render_type
end

function EquipmentPieceComponent:get_injected_ai()
   return self._info.injected_ai
end

function EquipmentPieceComponent:get_injected_commands()
   return self._info.injected_commands or {}
end


return EquipmentPieceComponent