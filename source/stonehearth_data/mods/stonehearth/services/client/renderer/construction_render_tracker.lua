local ConstructionRenderTracker = class()
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
local INFINITE = 100000000
local RPG_REGION_HEIGHT = 2

-- create a new construction render tracker for the specified entity.  the entity is used
-- during visibility calculations in xrag mode.
--
--    @param entity - the entity being rendererd.
--
function ConstructionRenderTracker:__init(entity)
   self._entity = entity
   self._build_mode_visible = false
   self._ui_mode_visible = true
   self._mode = nil
   
   self._mode_listener = radiant.events.listen(radiant, 'stonehearth:ui_mode_changed', self, self._on_ui_mode_changed)
   self._vision_mode_listener = radiant.events.listen(radiant, 'stonehearth:building_vision_mode_changed', self, self._on_building_visions_mode_changed)
end

function ConstructionRenderTracker:start()
   self:_on_building_visions_mode_changed()
   return self
end

-- destroy the construction render tracker.  
function ConstructionRenderTracker:destroy()
   self._mode_listener:destroy()
   self._mode_listener = nil

   self._vision_mode_listener:destroy()
   self._vision_mode_listener = nil

   if self._camera_listener then
      self._camera_listener:destroy()
      self._camera_listener = nil
   end
end

-- called with an argument list of all the ui modes for which the structure would like
-- to be visible.  it's invisible in all other modes.
--
--    @param ... - var args of string, one for each mode
--
function ConstructionRenderTracker:set_visible_ui_modes(...)
   local args = {...}
   if #args then
      self._visible_modes = {}
      for _, mode in ipairs(args) do
         self._visible_modes[mode] = true
      end
   else
      self._visible_modes = nil
   end
   local mode = stonehearth.renderer:get_ui_mode()   
   self._ui_mode_visible = self._visible_modes[mode] ~= nil   
   return self
end

-- notifies the construction render tracker that the region being rendered has changed.
-- may fire your region changed callback if this changes what should be rendered
--
--    @param region - the new shape of your building
--
function ConstructionRenderTracker:set_region(region)
   self._rpg_region_dirty = true
   self._region = region
   self:_fire_on_region_changed()
   return self
end

-- notifies the construction render tracker of the direction of the normal for your
-- building.  in xray mode, the direction and of the normal, the position of the
-- entity, and the position and direction of the camera are used to determine what
-- structures to render in ghost mode.
--
--    @param normal - Your normal as a Point3
--
function ConstructionRenderTracker:set_normal(normal)
   if normal then
      self._normal = Point3f(normal.x, normal.y, normal.z)
   end
   return self   
end

function ConstructionRenderTracker:set_type(type)
   self._type = type
   return self   
end

-- sets the function to call whenever the region for your structure to be rendered
-- changes.  this may be different that the region you specified in the :set_region()
-- function.  for example, in rpg mode we only show the very bottoms of structures
-- instead of the whole region.  `cb` will be passed the region to render, whether
-- or not it's visible, and the current ui mode.
--
--    @param cb - the function to call whenever the region changes.
--
function ConstructionRenderTracker:set_render_region_changed_cb(cb)
   self._on_region_changed = cb
   return self   
end

-- sets the function to call whenever the visibility of your structure needs to
-- change.  For example, in xray mode your structure may be hidden to reveal the
-- contents inside a room.  `cb` will be called with a bool representing whether
-- or not your structure is visible now.
--
--    @param cb - the function to call when the visibility changes.
--
function ConstructionRenderTracker:set_visible_changed_cb(cb)
   self._on_visible_changed = cb
   return self   
end

-- used to manually fire all callbacks.  useful for pushing initial render state
-- to yourself immediately after setting up the tracker.
--
function ConstructionRenderTracker:push_object_state()
   self:_fire_on_region_changed()

   if self._on_visible_changed then
      self._on_visible_changed(self._visible)
   end
end


function ConstructionRenderTracker:push_visible_state()
   if self._on_visible_changed then
      self._on_visible_changed(self._visible)
   end
end


-- computes the region to render in rpg mode.  this is just the normal render
-- region with all but the bottom-most voxels clipped off.  the result is cached
-- in self._rpg_region, and will only be recompouted when the user gives us
-- another region via :set_region()
--
function ConstructionRenderTracker:_compute_rpg_render_region()
   if not self._rpg_region then
      self._rpg_region = _radiant.client.alloc_region3()
   end
   if self._rpg_region_dirty then
      local clipper = Cube3(Point3(-INFINITE, 0, -INFINITE),
                            Point3( INFINITE, RPG_REGION_HEIGHT,  INFINITE))
      self._rpg_region:modify(function(cursor)
            cursor:copy_region(self._region:get():to_int():clipped(clipper))
         end)
      self._rpg_region_dirty = false
   end
   return self._rpg_region
end

-- computes the region the client should render and fires the on_region_changed
-- callback with it.  
--
function ConstructionRenderTracker:_fire_on_region_changed()
   if self._region then 
      if self._region:get():empty() then
         self._render_region = nil
      elseif self._mode == 'rpg' then
         self._render_region = self:_compute_rpg_render_region()
      else
         self._render_region = self._region
      end
   end   

   if self._on_region_changed then
      self._on_region_changed(self._render_region, self._visible, self._mode)
   end
end

-- called whenever the ui mode of the client changes.  notifies the client of the
-- new render state for their structure
--
function ConstructionRenderTracker:_on_ui_mode_changed()
   local mode = stonehearth.renderer:get_ui_mode()
   self._ui_mode_visible = not self._visible_modes or self._visible_modes[mode] ~= nil
   self:_update_visible()
end

-- called whenever the building view  mode of the client changes.  notifies the client
-- of the new render state for their structure
--
function ConstructionRenderTracker:_on_building_visions_mode_changed()
   local mode = stonehearth.renderer:get_building_vision_mode()
   if mode ~= self._mode then
      local last_mode = self._mode
      self._mode = mode

      -- roofs are only visible in normal mode.
      if self._type == 'roof' then
         self._build_mode_visible = self._mode ~= 'rpg' and self._mode ~= 'xray'
      elseif self._mode == 'xray' then
         -- if we're going into xray mode, start listening to camera changes so we can update
         -- the render state appropriately.  if not, remove the listener and double check our
         -- visible state.
         if not self._camera_listener then
            self._camera_listener = radiant.events.listen(stonehearth.camera, 'stonehearth:camera:update', self, self._update_camera)
         end
         self:_update_camera()
      else
         if self._camera_listener then
            self._camera_listener:destroy()
            self._camera_listener = nil
         end
         self._build_mode_visible = true
      end

      -- if we're entering or leaving rpg mode, the region has almost certainly changed.
      -- check up on it.
      if self._mode == 'rpg' or last_mode == 'rpg' then
         self:_fire_on_region_changed()
      end

      self:_update_visible()
   end
end

-- updates our visible state and fires the callback if it actually changed.
--
function ConstructionRenderTracker:_update_visible()
   local visible = self._build_mode_visible and self._ui_mode_visible
   if self._visible ~= visible then
      self._visible = visible
      if self._on_visible_changed then
         self._on_visible_changed(visible)
      end
      return true
   end
   return false
end

-- checks the position of the camera vs the position of the entity to see if
-- we should be rendered in xray mode.  updates the visibilty state appropriately.
--
function ConstructionRenderTracker:_update_camera()
   self._build_mode_visible = true
   local mode = stonehearth.renderer:get_building_vision_mode()
   if mode == 'xray' then
      if self._normal then
         local mob_location = self._entity:get_component('mob'):get_world_location()

         -- It could be the case that we get a callback while we're being removed from the world.
         if mob_location then
            -- move the camera relative to the mob
            local x0 = stonehearth.camera:get_position() - mob_location

            -- Are we in front of, or behind, the object's plane?
            self._build_mode_visible = self._normal:dot(x0) < 0
         end
      end
   end
   self:_update_visible()
end

return ConstructionRenderTracker

