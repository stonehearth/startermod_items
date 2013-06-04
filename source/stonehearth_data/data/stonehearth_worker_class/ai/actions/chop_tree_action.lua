local ChopTreeAction = class()

ChopTreeAction.name = 'stonehearth.actions.chop_tree'
ChopTreeAction.does = 'stonehearth.activities.top'
ChopTreeAction.priority = 0


function ChopTreeAction:__init(ai, entity)
   --[[
   local inventory = radiant.mods.require('mod://stonehearth_inventory')

   inventory:find_path_to_tree(entity, function(entity, path, tree)
         self._path = path
         self._tree = tree
         ai:set_priority(self, path:distance())
      end)
      ]]
end

function ChopTreeAction:run(ai, entity)
   assert(self._path and self._tree)

   ai:execute('stonehearth.activities.follow_path', self._path)
   radiant.entities.turn_to_face(entity, self._tree)
   ai:execute('stonehearth.activities.run_effect', 'chop')
   
   -- spawn the tree...
   assert(false)

   self._path = nil
   self._tree = nil
   ai:set_priority(self, 0)
end

return ChopTreeAction