--[[
   This component stores data about the job that a settler has.
]]

local JobInfo = class()

function JobInfo:__init()
   self._info = {}
end

function JobInfo:extend(json)
   self:set_info(json)
end

function JobInfo:set_info(info)
   self._info = info
end

function JobInfo:get_name()
   return self._info.name
end

function JobInfo:get_job_type()
   return self._info.job_type
end

function JobInfo:get_class_script()
   return self._info.class_script
end

return JobInfo
