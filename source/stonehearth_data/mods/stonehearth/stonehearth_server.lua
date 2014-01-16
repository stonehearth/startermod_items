function get_service(name)
   local service_name = string.format('services.%s.%s_service', name, name)
   return require(service_name)
end

stonehearth = {}
stonehearth.ai = get_service('ai')
stonehearth.events = get_service('event')
stonehearth.calendar = get_service('calendar')
stonehearth.combat = get_service('combat')
stonehearth.personality = get_service('personality')
stonehearth.inventory = get_service('inventory')
stonehearth.population = get_service('population')
stonehearth.object_tracker = get_service('object_tracker')
stonehearth.world_generation = get_service('world_generation')
stonehearth.build = get_service('build')
stonehearth.game_master = get_service('game_master')
stonehearth.analytics = get_service('analytics')
stonehearth.tasks = get_service('tasks')

return stonehearth
