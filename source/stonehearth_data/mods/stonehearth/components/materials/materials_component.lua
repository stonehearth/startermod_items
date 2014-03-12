--[[
   Todo, document this thing
]]

local MaterialsComponent = class()

function MaterialsComponent:__init()
   self._materials = {}
end

function MaterialsComponent:initialize(entity, data_binding)
   self.__saved_variables = radiant.create_datastore({
         materials = self._materials 
      })

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
   self.__saved_variables:mark_changed()
end

function MaterialsComponent:remove_material(material)
   self._materials[material] = nil
   self.__saved_variables:mark_changed()
end


return MaterialsComponent
