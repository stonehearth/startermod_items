local SelectionCallHandler = class()

function SelectionCallHandler:select_entity(session, response, entity)
   stonehearth.selection:select_entity(entity)
   response:resolve({})
end

return SelectionCallHandler
