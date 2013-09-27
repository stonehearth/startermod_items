local PlayWithBallAction = class()

PlayWithBallAction.name = 'stonehearth.actions.play_with_ball_action'
PlayWithBallAction.does = 'stonehearth.activities.top'
PlayWithBallAction.priority = 99

function PlayWithBallAction:__init(ai, entity, item)
   self._item = item
end

function PlayWithBallAction:run(ai, entity)
   ai:execute('stonehearth.activities.pickup_item', self._item)
end

return PlayWithBallAction