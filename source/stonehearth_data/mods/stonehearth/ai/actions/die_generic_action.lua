local log = radiant.log.create_logger('death')

local DieGeneric = class()

DieGeneric.name = 'die generic'
DieGeneric.does = 'stonehearth:die'
DieGeneric.args = {}
DieGeneric.version = 2
DieGeneric.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(DieGeneric)
   :execute('stonehearth:drop_carrying_now')
   :execute('stonehearth:destroy_entity')
