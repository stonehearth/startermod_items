local Region3 = _radiant.csg.Region3
local TraceCategories = _radiant.dm.TraceCategories

local FabricatorClientComponent = class()

-- this is the component which manages the fabricator entity.
function FabricatorClientComponent:initialize(entity, json)
   self._entity = entity
end

function FabricatorClientComponent:destroy()
   if self._blueprint_trace then
      self._blueprint_trace:destroy()
      self._blueprint_trace = nil
   end
   if self._project_trace then
      self._project_trace:destroy()
      self._project_trace = nil
   end
end

function FabricatorClientComponent:get_data()
   return self.__saved_variables:get_data()
end

function FabricatorClientComponent:get_blueprint()
   return self:get_data().blueprint
end

function FabricatorClientComponent:get_project()
   return self:get_data().project
end

function FabricatorClientComponent:get_editing_region()
   return self:get_data().editing_region
end

function FabricatorClientComponent:get_total_mining_region()
   return self:get_data().total_mining_region
end

function FabricatorClientComponent:begin_editing(blueprint, project, editing_region)
   -- xxx: assert we're in a RW store...
   self.__saved_variables:modify(function (o)
         o.blueprint = blueprint
         o.project = project
         o.editing_region = editing_region
         o.editing = true
      end)
      
   local function trace_region(entity)
      return entity:get_component('destination'):trace_region('editing fab', TraceCategories.SYNC_TRACE)
         :on_changed(function()
            local br = blueprint:get_component('destination'):get_region():get()
            local pr = project and project:get_component('destination'):get_region():get() or Region3()
            self._entity:get_component('region_collision_shape'):get_region():modify(function(cursor)
                  cursor:copy_region(br - pr)
               end)
         end)
   end     
   if project then
      self._project_trace = trace_region(project)
   end
   self._blueprint_trace = trace_region(blueprint)
   self._blueprint_trace:push_object_state()
end

return FabricatorClientComponent
