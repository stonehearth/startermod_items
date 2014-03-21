local ConstructionProgress = class()
local Point2 = _radiant.csg.Point2
local Region3 = _radiant.csg.Region3
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build')

function ConstructionProgress:initialize(entity, json)
   self._entity = entity
   self._sv = self.__saved_variables:get_data()
   if not self._sv.dependencies then
      self._sv.dependencies = {}
      self._sv.dependencies_finished = false
   end
end

function ConstructionProgress:get_dependencies()
   return self._sv.dependencies
end


--- Tracks projects which must be completed before this one can start.
-- This is actually an enormous pain.  The stonehearth:construction_data component of our blueprint
-- contains a list of dependencies to other *blueprints*.  That's not very useful: we need to know
-- the actual projects which are being built from the blueprint.  That information is stored in the
-- stonehearth:fabricator component attached to the blueprint.  Unfortunately, we're not guaranteed
-- all the dependencies have been started at the time _initialize_dependencies() gets called.  If
-- the stonehearth:fabricator does not yet exist, how are we supposed to get the blueprint?
--
-- So we use this two step process: first, listen for stonehearth:construction_fabricator_changed
-- notifications on the blueprint.  When those fire, we'll have the fabricator and can ask *it*
-- for the project.  In the interval between now and when we actually get the project, we assume
-- that the project has not been completed.
function ConstructionProgress:add_dependency(blueprint)
   local id = blueprint:get_id()

   -- Listen for changes to the blueprint's fabricator entity so we can find the actual
   -- project for this blueprint
   radiant.events.listen(blueprint, 'stonehearth:construction_fabricator_changed', self, self._on_construction_fabricator_changed)

   -- Create a new dependency tracking the blueprint. 
   self._sv.dependencies[id] = {
      blueprint = blueprint
   }

   -- Try to find the right project. We might not be able to if the blueprint hasn't
   -- actually been started yet.  If it hasn't, the change notification above will fire
   -- when it has and we'll get the project then.
   local cd = blueprint:get_component('stonehearth:construction_data')
   if cd then
      local fabricator = cd:get_fabricator_entity()
      self:_update_dependency_fabricator(blueprint, fabricator)
   end
   self.__saved_variables:mark_changed()
   self:check_dependencies()
end

function ConstructionProgress:_on_construction_fabricator_changed(e)
   local id = e.entity:get_id()
   local dependency = self._sv.dependencies[id]
   if dependency then
      self:_update_dependency_fabricator(e.entity, e.fabricator_entity)
   end
end

--- Wires up the actual project for the specified blueprint
function ConstructionProgress:_update_dependency_fabricator(blueprint, fabricator)
   if blueprint and fabricator then
      local id = blueprint:get_id()
      local dependency = self._sv.dependencies[id]
      if dependency then
         local fc = fabricator:get_component('stonehearth:fabricator')
         if fc then
            local project = fc:get_project()
            if dependency.project then
               assert(dependency.project:get_id() == project:get_id(), 'dependency project change!  ug!!')
            else 
               -- Yay!  we've finally gotten the project for this dependency.  Listen for
               -- construction_finished notifications so we can keep up-to-date and see
               -- if we're ready to build.
               dependency.project = project
               radiant.events.listen(project, 'stonehearth:construction_finished',
                                     self, self._on_construction_finished)

               self.__saved_variables:mark_changed()
               self:check_dependencies()
            end
         end
      end
   end
end

function ConstructionProgress:_on_construction_finished(e)
   self:check_dependencies()
end

function ConstructionProgress:check_dependencies()
   local last_dependencies_finished = self._sv.dependencies_finished
   
   -- Assume we're good to go.  If not, the loop below will catch it.
   self._sv.dependencies_finished = true
   for id, dependency in pairs(self._sv.dependencies) do
      local project = dependency.project
      if not project then
         -- We haven't gotten a project for this dependency yet, most likely because
         -- *this* project got processed by the city_plan before the dependency.  No
         -- biggie, we'll pick it up soon in a stonehearth:construction_fabricator_changed
         -- notification.  For now, assume the project is not finished and bail.
         self._sv.dependencies_finished = false
         break
      end
      local progress = project:get_component('stonehearth:construction_progress')
      if progress and not progress:get_finished() then
         -- The project is actually not finished.  Definitely bail!
         self._sv.dependencies_finished = false
         break
      end
   end
   
   -- Start or stop the project if the _dependencies_finished bit has flipped.
   if last_dependencies_finished ~= self._sv.dependencies_finished then
      radiant.events.trigger(self._entity, 'stonehearth:construction_dependencies_finished', { 
         entity = self._entity,
         finished = self._sv.dependencies_finished
      })
      -- if we don't have a construction_data component, forward the message along.
      local construction_data = self._entity:get_component('stonehearth:construction_data')
      if not construction_data then
         self:set_finished(self._sv.dependencies_finished)
      end
   end
   return self._sv.dependencies_finished
end

-- used by the fabricator to indicate the construction is done
function ConstructionProgress:set_finished(finished)
   local changed = self._sv.finished ~= finished
   if changed then
      self._sv.finished = finished
      self.__saved_variables:mark_changed()

      log:debug('%s trigger stonehearth:construction_finished event (finished = %s)',
                  self._entity, tostring(finished))

      radiant.events.trigger(self._entity, 'stonehearth:construction_finished', { 
         entity = self._entity,
         finished = finished
      })
   end
   return self
end

function ConstructionProgress:get_finished()
   return self._sv.finished
end

return ConstructionProgress
