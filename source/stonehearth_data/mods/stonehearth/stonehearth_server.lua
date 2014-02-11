function get_service(name)
   local service_name = string.format('services.%s.%s_service', name, name)
   return require(service_name)
end

stonehearth = {}

stonehearth.constants = require 'constants'
stonehearth.ai = get_service('ai')
stonehearth.events = get_service('event')
stonehearth.calendar = get_service('calendar')
stonehearth.combat = get_service('combat')
stonehearth.substitution = get_service('substitution')
stonehearth.personality = get_service('personality')
stonehearth.inventory = get_service('inventory')
stonehearth.scenario = get_service('scenario')
stonehearth.population = get_service('population')
stonehearth.object_tracker = get_service('object_tracker')
stonehearth.world_generation = get_service('world_generation')
stonehearth.build = get_service('build')
stonehearth.game_master = get_service('game_master')
stonehearth.analytics = get_service('analytics')
stonehearth.tasks = get_service('tasks')
stonehearth.threads = get_service('threads')
stonehearth.town = get_service('town')

return stonehearth
