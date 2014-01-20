local controller = require 'services.character_controller.character_controller_service'

local CharacterControllerCallHandler = class()

function CharacterControllerCallHandler:set_players(session, response, p1_id, p2_id)
   controller:set_players(p1_id, p2_id);
end

function CharacterControllerCallHandler:do_action(session, response, player_num, action, response)
   controller:do_action(player_num, action, response);
end

return CharacterControllerCallHandler
