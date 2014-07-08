local log = radiant.log.create_logger('death')

local DieCitizen = class()

DieCitizen.name = 'die citizen'
DieCitizen.does = 'stonehearth:die'
DieCitizen.args = {}
DieCitizen.version = 2
DieCitizen.priority = 2

local ai = stonehearth.ai
return ai:create_compound_action(DieCitizen)
   :execute('stonehearth:drop_carrying_now')
   :execute('stonehearth:memorialize_death')
   :execute('stonehearth:destroy_entity')
