--[[
   Todo, document this thing
]]

local MaterialsComponent = class()

function MaterialsComponent:__init(entity, data_binding)
   self._data_binding = data_binding
   self._materials = {}
   self._data = { 
      materials = self._materials 
   }
   self._data_binding:update(self._data)
end

function MaterialsComponent:extend(json)
   if json then
      for i, v in ipairs(json.materials) do
         self:add_material(v)
      end
   end
end

function MaterialsComponent:has_material(material)
   return self._materials[material]
end

function MaterialsComponent:add_material(material)
   self._materials[material] = true
   self._data_binding:mark_changed()
end

function MaterialsComponent:remove_material(material)
   self._materials[material] = nil
   self._data_binding:mark_changed()
end


return MaterialsComponent
