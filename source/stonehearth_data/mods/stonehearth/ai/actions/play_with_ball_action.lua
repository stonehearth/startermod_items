local PlayWithBallAction = class()

PlayWithBallAction.name = 'play with a ball'
PlayWithBallAction.does = 'stonehearth.top'
PlayWithBallAction.priority = 99

function PlayWithBallAction:__init(ai, entity, item)
   self._item = item
end

function PlayWithBallAction:run(ai, entity)
   ai:execute('stonehearth.pickup_item', self._item)
end

return PlayWithBallAction