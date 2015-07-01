local priorities = require('constants').priorities.worker_task
local constants = require 'constants'

local StorageComponent = class()
local log = radiant.log.create_logger('storage')

local INFINITE = 1000000
local ALL_FILTER_FNS = {}


function _can_steal_from_other_storage(auto_fill_from, item, container)
   local other_storage = container:get_component('stonehearth:storage')
   local other_container_type = other_storage:get_type()

   assert(other_storage:contains_item(item:get_id()))
   
   -- cannot steal from non-public storage
   if not other_storage:is_public() then
      return false
   end

   -- ok to steal from containers matching our auto_fill_from setting
   if auto_fill_from[other_container_type] then
      return true
   end

   -- ok to steal from containers if the item in question doesn't match
   -- their filter (e.g. the user changed the filter after the item was
   -- put into storage)
   -- take from the same container type if the filters don't match
   if not other_storage:passes(item) then
      return true
   end

   -- nope!  cannot steal that item from that storage
   return false
end

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


local function get_filter_fn(filter_key, filter, auto_fill_from, player_id, player_inventory)
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
      local captured_auto_fill_from = {}
      for k, v in pairs(auto_fill_from) do
         captured_auto_fill_from[k] = v
      end

      -- now create the filter function.  again, this function must work for
      -- *ALL* containers with the same filter key, which is why this is
      -- implemented in terms of global functions, parameters to the filter
      -- function, and captured local variables.
      filter_fn = function(item, options)
         log:detail('calling filter function on %s (key:%s)', item, filter_key)

         local item_player_id = radiant.entities.get_player_id(item)
         if item_player_id ~= player_id then
            log:detail('item player id "%s" ~= container id "%s".  returning from filter function', item_player_id, player_id)
            return false
         end
   
         -- the filter function is used both by the BFS pathfinder to consider items
         -- and by callers specifically to test an item (e.g. the 'pickup item type from
         -- backpack' function will be provided a filter from a stockpile when restocking).
         -- in the latter case, we allow the user to pass in some extra options to control
         -- the result.  if we're grabbing an item out of a backpack specifically, we need
         -- to turn off the stealing logic!  again, in the BFS pathfinder case where we have
         -- no specific use case in mind, the anti-stealing logic will prevent us from grabbing
         -- items from one time of storage when the 'auto_fill_from' mask forbids it. - tony
         local always_allow_stealing = options and options.always_allow_stealing 
         if not always_allow_stealing then
            -- If this item is already in a container for the player, then ignore it
            -- if its type is not in our autofill map
            local container = player_inventory:container_for(item)
            if container then
               if not _can_steal_from_other_storage(captured_auto_fill_from, item, container) then
                  return false
               end
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

   -- We don't hold reservations across saves.
   self.num_reserved = 0

   if not self._sv.entity then
      -- creating...
      local basic_tracker = radiant.create_controller('stonehearth:basic_inventory_tracker')
      self._sv.entity = entity
      self._sv.capacity = json.capacity or 8
      self._sv.num_items = 0
      self._sv.items = {}
      self._sv.passed_items = {}
      self._sv.filtered_items = {}
      self._sv.item_tracker = radiant.create_controller('stonehearth:inventory_tracker', basic_tracker)
      self._sv.type = json.type or "crate"

      self._sv.auto_fill_from = {}
      for _, v in pairs(json.auto_fill_from or {}) do
         self._sv.auto_fill_from[v] = true
      end
      
      if json.public ~= nil then
         self._sv.is_public = json.public
      else
         self._sv.is_public = true
      end
      
      if self._sv.is_public then
          local default_filter_none = radiant.util.get_config('default_storage_filter_none', false)
          if default_filter_none then
            self:set_filter({})
          end
      end
      
      self.__saved_variables:mark_changed()
   end

   self._kill_listener = radiant.events.listen(entity, 'stonehearth:kill_event', self, self._on_kill_event)

   self:_on_contents_changed()
   
   self._unit_info_trace = entity:add_component('unit_info')
                                    :trace_player_id('filter observer')
                                       :on_changed(function()
                                             self:_update_player_id()
                                          end)
   self._unit_info_trace:push_object_state()
end

--If we're killed, dump the things in our backpack
function StorageComponent:_on_kill_event()
   -- npc's don't drop what's in their pack
   if not stonehearth.player:is_npc(self._sv.entity) then
      while not self:is_empty() do
         local item = self:remove_first_item()
         local location = radiant.entities.get_world_grid_location(self._sv.entity)
         local placement_point = radiant.terrain.find_placement_point(location, 1, 4)
         radiant.terrain.place_entity(item, placement_point)
      end
   end
end

function StorageComponent:destroy()
   self._unit_info_trace:destroy()
   self._unit_info_trace = nil

   for id, _ in pairs(self._sv.items) do
      self:remove_item(id)
   end

   if self._kill_listener then
      self._kill_listener:destroy()
      self._kill_listener = nil
   end

   self._sv.items = nil
end

function StorageComponent:reserve_space()
   if self:get_num_items() + self.num_reserved >= self:get_capacity() then
      return false
   end
   self.num_reserved = self.num_reserved + 1
   return true
end

function StorageComponent:unreserve_space()
   if self.num_reserved > 0 then
      self.num_reserved = self.num_reserved - 1
   end
end

-- Call to increase/decrease backpack size
-- @param capacity_change - add this number to capacity. For decrease, us a negative number
function StorageComponent:change_max_capacity(capacity_change)
   self._sv.capacity = self:get_capacity() + capacity_change
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

function StorageComponent:add_item(item)
   if self:is_full() then
      return false
   end

   local id = item:get_id()

   if self._sv.items[id] then
      return true
   end

   self._sv.items[id] = item
   self._sv.num_items = self._sv.num_items + 1
   self:_filter_item(item)
   self._sv.item_tracker:add_item(item, self._sv.entity)

   local player_id = self._sv.entity:get_component('unit_info')
                                       :get_player_id()

   local inventory = stonehearth.inventory:get_inventory(player_id)
   if inventory then
      inventory:add_item(item, self._sv.entity)
   end
   
   if item:is_valid() then
      radiant.events.trigger_async(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', item)
   end
   self:_on_contents_changed()
   self.__saved_variables:mark_changed()

   radiant.events.trigger_async(self._sv.entity, 'stonehearth:storage:item_added', {      
         item = item,
         item_id = item:get_id(),
      })
end

function StorageComponent:remove_item(id)
   assert(type(id) == 'number', 'expected entity id')
   
   local item = self._sv.items[id] 
   if not item then
      return nil
   end
   
   self._sv.num_items = self._sv.num_items - 1
   self._sv.items[id] = nil
   self._sv.passed_items[id] = nil
   self._sv.filtered_items[id] = nil
   self._sv.item_tracker:remove_item(id)

   local player_id = self._sv.entity:get_component('unit_info')
                                       :get_player_id()
   local inventory = stonehearth.inventory:get_inventory(player_id)
   if inventory then
      inventory:update_item_container(id)
   end

   if item:is_valid() then
      radiant.events.trigger_async(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', item)
   end

   self:_on_contents_changed()
   self.__saved_variables:mark_changed()

   radiant.events.trigger_async(self._sv.entity, 'stonehearth:storage:item_removed', {      
         item_id = id,
      })
   return item
end

function StorageComponent:contains_item(id)
   checks('self', 'number')
   return self._sv.items[id] ~= nil
end

function StorageComponent:get_items()
   return self._sv.items
end

function StorageComponent:num_items()
   return self._sv.num_items
end

function StorageComponent:get_capacity()
   return self._sv.capacity or INFINITE
end

function StorageComponent:is_public()
   return self._sv.is_public
end

function StorageComponent:set_capacity(value)
   self._sv.capacity = value
   self.__saved_variables:mark_changed()
end

function StorageComponent:is_empty()
   return self._sv.num_items == 0
end

function StorageComponent:is_full()
   return self._sv.num_items >= self:get_capacity()
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
   local commands_component = self._sv.entity:get_component('stonehearth:commands')
   if commands_component then
      commands_component:enable_command('undeploy_item', self:is_empty())
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
   local player_id = self._sv.entity:get_component('unit_info')
                                       :get_player_id()

   return get_filter_fn(
      self._sv._filter_key,
      self._sv.filter, 
      self._sv.auto_fill_from,
      player_id, 
      stonehearth.inventory:get_inventory(player_id))
end

function StorageComponent:get_filter()
   return self._sv.filter
end

function StorageComponent:set_filter(filter)
   local player_id = self._sv.entity:add_component('unit_info')
                                       :get_player_id()

   self._sv.filter = filter
   self:_update_filter_key(player_id)

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
            radiant.events.trigger_async(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', item)
         end
      elseif old_filtered[id] and self._sv.passed_items[id] then
         newly_passed[id] = item
         if item:is_valid() then
            radiant.events.trigger_async(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', item)
         end
      end
   end

   self.__saved_variables:mark_changed()
   
   -- Let the AI know that _we_ (the backpack) have changed, so reconsider us, too!
   radiant.events.trigger_async(stonehearth.ai, 'stonehearth:pathfinder:reconsider_entity', self._sv.entity)
   radiant.events.trigger_async(self._sv.entity, 'stonehearth:storage:filter_changed', self, newly_filtered, newly_passed)
end

function StorageComponent:_update_player_id()
   local player_id = self._sv.entity:add_component('unit_info')
                                       :get_player_id()

   -- xxx: we also need to add/remove all the items in the
   -- box to the new/old inventory
   self:_update_inventory(player_id)
   self:_update_filter_key(player_id)
end

function StorageComponent:_update_inventory(player_id)
   local inventory = stonehearth.inventory:get_inventory(player_id)
   if inventory then
      inventory:add_storage(self._sv.entity)
   end
end

function StorageComponent:_update_filter_key(player_id)
   if self._sv.filter then
      self._sv._filter_key = 'filter:'
      table.sort(self._sv.filter)
      for _, material in ipairs(self._sv.filter) do
         self._sv._filter_key = self._sv._filter_key .. '+' .. material
      end
   else
      self._sv._filter_key = 'nofilter'
   end

   self._sv._filter_key = self._sv._filter_key .. '+' .. player_id .. '+' .. self._sv.type .. '+autofill='
   for k, _ in pairs(self._sv.auto_fill_from) do
      self._sv._filter_key = self._sv._filter_key .. '+' .. k 
   end
   self.__saved_variables:mark_changed()   
end

return StorageComponent
