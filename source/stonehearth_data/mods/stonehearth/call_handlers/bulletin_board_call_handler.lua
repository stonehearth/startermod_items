local BulletinBoardCallHandler = class()

function BulletinBoardCallHandler:get_bulletin_board_datastore(session, response)
   local bulletin_board = stonehearth.bulletin_board:get_datastore(session.player_id)
   return { bulletin_board = bulletin_board }
end

function BulletinBoardCallHandler:remove_bulletin(session, response, bulletin_id)
   stonehearth.bulletin_board:remove_bulletin(session.player_id, bulletin_id)
   response:resolve({})
end

function BulletinBoardCallHandler:trigger_bulletin_event(session, response, bulletin_id, event_name, ...)
   local result = stonehearth.bulletin_board:trigger_event(bulletin_id, event_name, ...)
   response:resolve({ result = result })
end

return BulletinBoardCallHandler
