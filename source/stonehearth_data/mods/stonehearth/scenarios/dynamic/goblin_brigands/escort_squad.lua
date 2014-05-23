local EscortSquad = class()
local Point3 = _radiant.csg.Point3


function EscortSquad:__init(player_id, saved_variables)
  self.__saved_variables = saved_variables
  self._sv = self.__saved_variables:get_data()
  self._player_id = player_id

  if not self._sv._escorts then
    self._sv._escorts = {}
    self._sv._escorted = nil
    self._sv._spawned = false
  elseif self._sv._spawned then
    self:_attach_listeners()
  end
end


function EscortSquad:set_escorted(escorted_type)
  -- For now, just in case I forget....
  assert (self._sv._escorted == nil)

  local escorted = stonehearth.population:get_population(self._player_id):create_new_citizen()
  self._sv._escorted = escorted

  self.__saved_variables:mark_changed()
  return escorted
end


function EscortSquad:add_escort(escort_type, weapon_uri)
  local escort = stonehearth.population:get_population(self._player_id):create_new_citizen()
  self:_equip_with_weapon(escort, weapon_uri)

  self._sv._escorts[escort:get_id()] = escort

  self.__saved_variables:mark_changed()
  return escort
end


function EscortSquad:place_squad(spawn_point)
  radiant.terrain.place_entity(self._sv._escorted, spawn_point)

  self:_attach_listeners()

  -- Simple box pattern for now.  Spiral out from the 'upper left' of the spawn point.

  local x = -2
  local z = -2
  local x_dir = 2
  local z_dir = 0
  local turn_size = 2
  local placed_this_turn = 0
  local turn_count = 0

  for _, escort in pairs(self._sv._escorts) do

    radiant.terrain.place_entity(escort, spawn_point + Point3(x, 0, z))

    if placed_this_turn == turn_size then
      placed_this_turn = 0
      turn_count = turn_count + 1
      if turn_count % 2 == 0 then
        turn_size = turn_size + 2
        if x_dir > 0 then
          x_dir = 0
          z_dir = -2
        elseif x_dir < 0 then
          x_dir = 0
          z_dir = 2
        elseif z_dir > 0 then
          z_dir = 0
          x_dir = 2
        else
          z_dir = 0
          x_dir = -2
        end
      end
    end

    x = x + x_dir
    z = z + z_dir

    placed_this_turn = placed_this_turn + 1
  end
  self._sv._spawned = true
  self.__saved_variables:mark_changed()
end


function EscortSquad:spawned()
  return self._sv._spawned
end

function EscortSquad:_attach_listeners()
  for _, escort in pairs(self._sv._escorts) do
    self:_add_escort_task(escort, self._sv._escorted)
  end

  radiant.events.listen_once(self._sv._escorted, 'radiant:entity:pre_destroy', self, self._escorted_destroyed)
  radiant.events.listen(self._sv._escorted, 'stonehearth:combat:engage', self, self._on_escorted_attacked)

  for _, escort in pairs(self._sv._escorts) do
    radiant.events.listen_once(escort, 'radiant:entity:pre_destroy', self, self._escort_destroyed)
  end

end

function EscortSquad:_on_escorted_attacked(context)
  for _, escort in pairs(self._sv._escorts) do
    local target_table = radiant.entities.get_target_table(escort, 'aggro')
    target_table:add(context.attacker)
  end
end


function EscortSquad:_escorted_destroyed(e)
  radiant.events.unlisten(self._sv._escorted, 'stonehearth:combat:engage', self, self._on_escorted_attacked)
  self._sv._escorted = nil

  self:_entity_destroyed()
end


function EscortSquad:_escort_destroyed(e)
  self._sv._escorts[e.entity_id] = nil

  self:_entity_destroyed()
end


function EscortSquad:_entity_destroyed()
  if #self._sv._escorts == 0 and self._sv._escorted == nil then
    radiant.events.trigger(self, 'stonehearth:squad:squad_destroyed')
  end
  self.__saved_variables:mark_changed()
end


function EscortSquad:_equip_with_weapon(entity, weapon_uri)
   local weapon = radiant.entities.create_entity(weapon_uri)
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item(weapon, 'melee_weapon')

   -- HACK: remove the talisman glow effect from the weapon
   -- might want to remove other talisman related commands as well
   -- TODO: make the effects and commands specific to the model variant
   weapon:remove_component('effect_list')
end


function EscortSquad:_add_escort_task(escort, escorted)
  escort:get_component('stonehearth:ai')
    :get_task_group('stonehearth:work')
    :create_task('stonehearth:follow_entity', { 
       target = escorted,
       follow_distance = 3
       })
    :set_name('escort guard task')
    :start()
end


return EscortSquad
