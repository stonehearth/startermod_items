local SleepWhenExhaused = class()
SleepWhenExhaused.name = 'sleep when exhausted'
SleepWhenExhaused.does = 'stonehearth:sleep'
SleepWhenExhaused.args = {}
SleepWhenExhaused.version = 2
SleepWhenExhaused.priority = 1


local ai = stonehearth.ai
return ai:create_compound_action(SleepWhenExhaused)
            :execute('stonehearth:wait_for_attribute_above', { attribute = 'sleepiness', value = stonehearth.constants.sleep.EXHAUSTION })
            :execute('stonehearth:sleep_exhausted')
