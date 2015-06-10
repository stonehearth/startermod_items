local Entity = _radiant.om.Entity
local Path = _radiant.sim.Path

local FindTrapInTrappingGrounds = class()

FindTrapInTrappingGrounds.name = 'find trap in trapping_grounds'
FindTrapInTrappingGrounds.status_text = 'checking traps'
FindTrapInTrappingGrounds.does = 'stonehearth:trapping:find_trap_in_trapping_grounds'
FindTrapInTrappingGrounds.args = {
   trapping_grounds = Entity
}
FindTrapInTrappingGrounds.think_output = {
   trap = Entity,
   path = Path,
}
FindTrapInTrappingGrounds.version = 2
FindTrapInTrappingGrounds.priority = 1

function FindTrapInTrappingGrounds:start_thinking(ai, entity, args)
   local backpack_component = entity:get_component('stonehearth:backpack')
   if not backpack_component or backpack_component:is_full() then
      return
   end

   local trapping_grounds_component = args.trapping_grounds:add_component('stonehearth:trapping_grounds')
   local traps = trapping_grounds_component:get_traps()

   if not next(traps) then
      return
   end

   self._pathfinder = _radiant.sim.create_astar_path_finder(entity, 'find trap in trapping_grounds')
   for id, trap in pairs(traps) do
      self._pathfinder:add_destination(trap)
   end

   self._pathfinder:set_solved_cb(function(path)
         ai:set_think_output({
            path = path,
            trap = path:get_destination()
         })
         return true
      end)
   self._pathfinder:start()
end

function FindTrapInTrappingGrounds:stop_thinking(ai, entity, args)
   if self._pathfinder then
      self._pathfinder:stop()
      self._pathfinder = nil
   end
end

return FindTrapInTrappingGrounds
