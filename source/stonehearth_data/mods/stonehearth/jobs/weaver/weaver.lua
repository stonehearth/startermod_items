local CraftingJob = require 'jobs.crafting_job'

local WeaverClass = class()
radiant.mixin(WeaverClass, CraftingJob)

return WeaverClass
