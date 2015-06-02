local CraftingJob = require 'jobs.crafting_job'

local CarpenterClass = class()
radiant.mixin(CarpenterClass, CraftingJob)

return CarpenterClass
