local priorities = require('constants').priorities.worker_task
local constants = require 'constants'

local StorageComponent = class()
local log = radiant.log.create_logger('storage')

local ALL_FILTER_FNS = {}

local function _filter_passes(entity, filter)
   if not entity or not entity:is_valid() then
      log:spam('%s is not a valid entity.  cannot be stored.', tostring(entity))
      return false
   end

   local efc = entity:get_component('stonehearth:entity_forms')
   if efc and efc:get_should_restock() then
      local iconic = efc:get_iconic_entity()
      return _filter_passes(iconic, filter) 
   end

   if not entity:get_component('item') then
      log:spam('%s is not an item material.  cannot be stored.', entity)
      return false
   end

   local material = entity:get_component('stonehearth:material')
   if not material then
      log:spam('%s has no material.  cannot be stored.', entity)
      return false
   end

   -- no filter means anything
   if not filter then
      log:spam('container has no filter.  %s item can be stored!', entity)
      return true
   end

   -- must match at least one material in the filter, or this cannot be stored
   for i, mat in ipairs(filter) do
      if material:is(mat) then
         log:spam('%s matches filter "%s" and can be stored!', entity, mat)
         return true
      end
   end

   log:spam('%s failed filter.  cannot be stored.', entity)
   return false
end


local function get_filter_fn(filter_key, filter, player_id, player_inventory, container_type)
   -- all containers with the same filter must use the same filter function
   -- to determine whether or not an item can be stored.  this function is
   -- uniquely identified by the filter key.  this allows us to use a
   -- shared 'stonehearth:pathfinder' bfs pathfinder to find items which should
   -- go in containers rather than creating 1 bfs pathfinder per-container
   -- per worker (!!)
   local filter_fn = ALL_FILTER_FNS[filter_key]

   if not filter_fn then
      -- no function for the current filter has been created yet, so let's make
      -- one.  capture the current value of the filter in the closure so the
      -- implementation won't change when someone changes our filter.
      local captured_filter
      if filter then
         captured_filter = {}
         for _, material in ipairs(filter) do
            table.insert(captured_filter, material)
         end
      end
      -- now create the filter function.  again, this function must work for
      -- *ALL* containers with the same filter key, which is why this is
      -- implemented in terms of global functions, parameters to the filter
      -- function, and captured local variables.
      filter_fn = function(item)
         log:detail('calling filter function on %s', item)

         local item_player_id = radiant.entities.get_player_id(item)
         if item_player_id ~= player_id then
            log:detail('item player id "%s" ~= container id "%s".  returning from filter function', item_player_id, player_id)
            return false
         end

         -- If this item is already in a container for the player, then ignore it.
         local container = player_inventory:container_for(item)
         if container then
            local other_storage = container:get_component('stonehearth:storage')
            local other_container_type = other_storage:get_type()
            if container_type == constants.container_types.CRATE then
               -- Crates do not restock from crates...
               if other_container_type == constants.container_types.CRATE then
                  -- ... unless those crates no longer store items of that type.
                  if other_storage:passes(item) then
                     return false
                  end
               end
            elseif container_type == constants.container_types.VAULT then
               -- Vaults do not restock from vaults or crates.
               if other_container_type ~= constants.container_types.BACKPACK then
                  return false
               end
            else
               -- Backpacks don't restock from _any_ containers.
               return false
            end
         end

         return _filter_passes(item, captured_filter)
      end

      -- remember the filter function for the future
      ALL_FILTER_FNS[filter_key] = filter_fn
   end
   return filter_fn   
end


function StorageComponent:initialize(entity, json)

   self._sv = self.__saved_variables:get_data()

   self._unit_info_trace = entity:add_component('unit_info'):trace_player_id('filter observer')
      :on_changed(function()
            self._sv.player_id = entity:add_component('unit_info'):get_player_id()
            self:_update_filter_key()
         end)


   if not self._sv.entity then
      -- creating...
      local basic_tracker = radiant.create_controller('stonehearth:basic_inventory_tracker')
      self._sv.entity = entity
      self._sv.num_items = 0
      self._sv.items = {}
      self._sv.passed_items = {}
      self._sv.filtered_items = {}
      self._sv.item_tracker = radiant.create_controller('stonehearth:inventory_tracker', basic_tracker)
      self._sv.player_id = entity:add_component('unit_info'):get_player_id()
      self._sv.type = json.type or constants.container_types.CRATE
      self.__saved_variables:mark_changed()
   end

   self:_on_contents_changed()
   self:_update_filter_key()
end

function StorageComponent:destroy()
   self._unit_info_trace:destroy()
   self._unit_info_trace = nil

   for id, _ in pairs(self._sv.items) do
      self:remove_item(id)
   end

   self._sv.items = nil
end

function StorageComponent:get_num_items()
   return self._sv.num_items
end

function StorageComponent:get_items()
   return self._sv.items
end

function StorageComponent:get_passed_items()
   return self._sv.passed_items
end

function StorageComponent:item_with_id(id)
   return self._sv.items[id]
end

function StorageComponent:add_item(item)
   local id = item:get_id()
   self._sv.items[id] = item
   self._sv.num_items = self._sv.num_items + 1
   self:_filter_item(item)
   self._sv.item_tracker:add_item(item)

   stonehearth.inventory:get_inventory(self._sv.player_id):add_item(item)
   stonehearth.inventory:get_inventory(self._sv.player_id):update_item_container(id, self._sv.entity)

   if item:is_valid() then
      radiant.events.trigger(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', item)
   end
   self:_on_contents_changed()

   self.__saved_variables:mark_changed()
end

function StorageComponent:remove_item(id)
   assert(self._sv.items[id])

   local item = self._sv.items[id]
   self._sv.num_items = self._sv.num_items - 1
   self._sv.items[id] = nil
   self._sv.passed_items[id] = nil
   self._sv.filtered_items[id] = nil
   self._sv.item_tracker:remove_item(id)

   stonehearth.inventory:get_inventory(self._sv.player_id):remove_item(id)
   stonehearth.inventory:get_inventory(self._sv.player_id):update_item_container(id, nil)
   if item:is_valid() then
      radiant.events.trigger(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', item)
   end

   self:_on_contents_changed()

   self.__saved_variables:mark_changed()
end

function StorageComponent:_filter_item(item)
   if self:passes(item) then
      self._sv.passed_items[item:get_id()] = item
   else
      self._sv.filtered_items[item:get_id()] = item
   end
end

function StorageComponent:_on_contents_changed()
   -- Crates cannot undeploy when they are carrying stuff.
   if self._sv.type == constants.container_types.CRATE then
      local bp = self._sv.entity:get_component('stonehearth:backpack')
      
      if bp then
         local commands_component = self._sv.entity:add_component('stonehearth:commands')
         commands_component:enable_command('undeploy_item', bp:is_empty())
      end
   end
end

function StorageComponent:passes(entity)
   return _filter_passes(entity, self._sv.filter)
end

function StorageComponent:get_type()
   return self._sv.type
end

-- returns the filter key and function used to determine whether an item can
-- be stored in the owning container.
function StorageComponent:get_filter_function()
   -- this intentionally delegates to a helper function to avoid the use of `self`
   -- in the filter (which must work for ALL containers sharing that filter, and
   -- therefore should not capture self or any members of self!)
   return get_filter_fn(
      self._sv._filter_key, 
      self._sv.filter, 
      self._sv.player_id, 
      stonehearth.inventory:get_inventory(self._sv.player_id),
      self._sv.type)
end

function StorageComponent:get_filter()
   return self._sv.filter
end

function StorageComponent:set_filter(filter)
   self._sv.filter = filter
   self:_update_filter_key()

   local old_passed = self._sv.passed_items
   local old_filtered = self._sv.filtered_items
   local newly_passed = {}
   local newly_filtered = {}
   self._sv.passed_items = {}
   self._sv.filtered_items = {}

   for id, item in pairs(self._sv.items) do
      self:_filter_item(item)

      if old_passed[id] and self._sv.filtered_items[id] then
         newly_filtered[id] = item
         if item:is_valid() then
            radiant.events.trigger(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', item)
         end
      elseif old_filtered[id] and self._sv.passed_items[id] then
         newly_passed[id] = item
         if item:is_valid() then
            radiant.events.trigger(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', item)
         end
      end
   end

   self.__saved_variables:mark_changed()

   radiant.events.trigger(self._sv.entity, 'stonehearth:storage:filter_changed', self, newly_filtered, newly_passed)
end

function StorageComponent:_update_filter_key()
   if self._sv.filter then
      self._sv._filter_key = 'filter:'
      table.sort(self._sv.filter)
      for _, material in ipairs(self._sv.filter) do
         self._sv._filter_key = self._sv._filter_key .. '+' .. material
      end
   else
      self._sv._filter_key = 'nofilter'
   end
   self._sv._filter_key = self._sv._filter_key .. '+' .. self._sv.player_id .. '+' .. self._sv.type
   self.__saved_variables:mark_changed()   
end

return StorageComponent
