local stonehearth_trees = radiant.mods.require('/stonehearth_trees')

local ChopTreeAction = class()

ChopTreeAction.name = 'stonehearth.actions.chop_tree'
ChopTreeAction.does = 'stonehearth.activities.top'
ChopTreeAction.priority = 0


function ChopTreeAction:__init(ai, entity)
   self._ai = ai
   self._entity = entity
   radiant.events.listen('radiant.events.gameloop', self)
end

-- xxx: this is a really common pattern... register for a callback
-- at the start of the game loop, do something, then unlisten.
-- what we really need is a one-time callback when the game is
-- "loaded" and ready to go.  we can't do this in the constructor
-- because, for example, the entire entity may not yet have been
-- assembled. 
ChopTreeAction['radiant.events.gameloop'] = function(self)
   self:_start_search()
   radiant.events.unlisten('radiant.events.gameloop', self)
end
   

function ChopTreeAction:_start_search()
   

   self._path = nil
   self._ai:set_action_priority(self, 0)
   
   stonehearth_trees.find_path_to_tree(self._entity, function(path)
         self._path = path
         self._ai:set_action_priority(self, 10)
      end)
end

function ChopTreeAction:run(ai, entity)
   assert(self._path)
   local tree = self._path:get_destination()

   ai:execute('stonehearth.activities.follow_path', self._path)
   radiant.entities.turn_to_face(entity, tree)
   ai:execute('stonehearth.activities.run_effect', 'chop')
   
   local factory = tree:get_component('radiant:resource_node')
   if factory then
      local location = radiant.entities.get_world_grid_location(entity)
      factory:spawn_resource(location)
   end
end

function ChopTreeAction:stop()
   self._ai:set_action_priority(self, 0)
   self:_start_search()
end

return ChopTreeAction

