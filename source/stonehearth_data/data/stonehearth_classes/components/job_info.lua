--[[
   This component stores data about the job that a settler has.
]]

local JobInfo = class()

function JobInfo:__init()
   self._name = nil
   self._description = nil
   self._icon = nil
   self._class_script = nil
end

function JobInfo:extend(json)
   if json then
      if json.name then
         self._name = json.name
      end
      if json.description then
         self._description = json.description
      end
      if json.icon then
         self._icon = json.icon
      end
      if json.class_script then
         self._class_script = json.class_script
      end
   end
end

function JobInfo:get_class_script()
   return self._class_script
end

return JobInfo
