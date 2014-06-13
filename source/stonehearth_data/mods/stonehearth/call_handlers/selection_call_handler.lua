local SelectionCallHandler = class()

function SelectionCallHandler:select_entity(session, response, entity)
   stonehearth.selection:select_entity(entity)
   response:resolve({})
end

function SelectionCallHandler:deactivate_all_tools(session, response)
   stonehearth.selection:deactivate_all_tools()
   response:resolve({})
end

return SelectionCallHandler
