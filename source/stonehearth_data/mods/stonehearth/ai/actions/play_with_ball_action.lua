local PlayWithBallAction = class()

PlayWithBallAction.name = 'play with a ball'
PlayWithBallAction.does = 'stonehearth:top'
PlayWithBallAction.args = {}
PlayWithBallAction.version = 2
PlayWithBallAction.priority = 1

function PlayWithBallAction:start_thinking(ai, entity, args)
   --TBD
end

function PlayWithBallAction:run(ai, entity, args)
   --ai:execute('stonehearth:pickup_item', self._item)
end

return PlayWithBallAction