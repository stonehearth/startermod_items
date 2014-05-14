local Population = stonehearth.Population

local ScoreCallHandler = class()

function ScoreCallHandler:get_score(session, response)
   return { score = stonehearth.score:get_scores_for_player(session.player_id) }
end

return ScoreCallHandler
