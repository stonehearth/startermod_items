local EatCarrying = class()
EatCarrying.name = 'eat carrying'
EatCarrying.does = 'stonehearth:eat_carrying'
EatCarrying.args = { }
EatCarrying.version = 2
EatCarrying.priority = 1

function EatCarrying:run(ai, entity)
   local food = radiant.entities.get_carrying(entity)
   if not food then
      ai:abort('cannot eat. not carrying anything!')
   end

   ai:execute('stonehearth:eat_item', {
      food = food,
   })
end

function EatCarrying:stop(ai, entity, args)
end

return EatCarrying
