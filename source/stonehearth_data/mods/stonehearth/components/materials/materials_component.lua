--[[
   Todo, document this thing
]]

local MaterialsComponent = class()

function MaterialsComponent:__init()
   self._materials = {}
end

function MaterialsComponent:__create(entity, data_binding)
   self.__savestate = radiant.create_datastore({
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
   self.__savestate:mark_changed()
end

function MaterialsComponent:remove_material(material)
   self._materials[material] = nil
   self.__savestate:mark_changed()
end


return MaterialsComponent
