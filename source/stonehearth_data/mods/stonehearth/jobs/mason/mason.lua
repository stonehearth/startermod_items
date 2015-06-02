local CraftingJob = require 'jobs.crafting_job'

local MasonClass = class()
radiant.mixin(MasonClass, CraftingJob)

return MasonClass
