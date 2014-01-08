local SleepInBed = class()

SleepInBed.name = 'sleep in bed'
SleepInBed.does = 'stonehearth:sleep'
SleepInBed.args = {}
SleepInBed.version = 2
SleepInBed.priority = 1

function SleepInBed:start_thinking(ai, entity, stockpile)
   assert(false)
end

return SleepInBed
