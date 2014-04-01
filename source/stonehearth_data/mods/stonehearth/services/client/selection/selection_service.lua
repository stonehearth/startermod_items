local log = radiant.log.create_logger('selection_service')

local SelectionService = class()

function SelectionService:initialize()
  stonehearth.input:push_handler(function(e)
    return self:_on_mouse_input(e)
  end)
end

function SelectionService:_on_mouse_input(e)

  if e:up(1) then
    local results = _radiant.client.query_scene(e.x, e.y)

    if results:is_valid() then
      local id = results:objectid_of(0)
      local e = _radiant.client.get_object(id)
      local tc = e:get_component('terrain')

      if tc ~= nil then
        _radiant.client.select_entity_by_id(0)
      else
        _radiant.client.select_entity_by_id(id)
      end
    else
      _radiant.client.select_entity_by_id(0)
    end
    return true
  end

  return false
end

return SelectionService