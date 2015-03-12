ifeq ($(MAKE_ROOT),)
-include local.mk
else
-include $(MAKE_ROOT)/../local.mk
endif

ifeq ($(BUILD_TYPE),)
  ifneq ($(BAMBOO_PRODUCT_BUILDTYPE),)
     BUILD_TYPE=$(BAMBOO_PRODUCT_BUILDTYPE)
  else
     BUILD_TYPE   ?= opt
  endif
endif

ifeq ($(BUILD_TYPE), release)
	MSBUILD_CONFIGURATION=Release
	MODULE_MSBUILD_CONFIGURATION=Release
endif
ifeq ($(BUILD_TYPE), opt)
	MSBUILD_CONFIGURATION=RelWithDebInfo
	MODULE_MSBUILD_CONFIGURATION=Release
endif
ifeq ($(BUILD_TYPE), debug)
	MSBUILD_CONFIGURATION=Debug
	MODULE_MSBUILD_CONFIGURATION=Debug
endif

ifeq (_$(MSBUILD_CONFIGURATION)_, __)
   $(error Unknown BUILD_TYPE=$(BUILD_TYPE). Must be debug, opt, or release.)
endif

ifneq ($(ENABLE_MEMPRO),)
	RADIANT_CMAKE_FLAGS := $(RADIANT_CMAKE_FLAGS) -DENABLE_MEMPRO=true
	RADIANT_MSBUILD_FLAGS := $(RADIANT_MSBUILD_FLAGS) -p:DefineConstants="ENABLE_MEMPRO"
endif

ifneq ($(NO_ASSERT),)
   RADIANT_CMAKE_FLAGS := $(RADIANT_CMAKE_FLAGS) -DNO_ASSERT=ON
   RADIANT_MSBUILD_FLAGS := $(RADIANT_MSBUILD_FLAGS) -p:DefineConstants="NO_ASSERT"
endif

ifneq ($(VTUNE_PATH),)
   RADIANT_CMAKE_FLAGS := $(RADIANT_CMAKE_FLAGS) -DVTUNE_PATH=\"$(VTUNE_PATH)\"
   RADIANT_VTUNE_FLAGS := $(RADIANT_MSBUILD_FLAGS) -p:DefineConstants="VTUNE_PATH=$(VTUNE_PATH)"
endif

ifneq ($(ENABLE_OBJECT_COUNTER),)
	RADIANT_CMAKE_FLAGS := $(RADIANT_CMAKE_FLAGS) -DENABLE_OBJECT_COUNTER=ON
	RADIANT_MSBUILD_FLAGS := $(RADIANT_MSBUILD_FLAGS) -p:DefineConstants="ENABLE_OBJECT_COUNTER"
endif

ifneq ($(DISABLE_WHOLE_PROGRAM_OPTIMIZATION),)
   RADIANT_CMAKE_FLAGS := $(RADIANT_CMAKE_FLAGS) -DDISABLE_WHOLE_PROGRAM_OPTIMIZATION=$(DISABLE_WHOLE_PROGRAM_OPTIMIZATION)
endif

RADIANT_CMAKE_FLAGS := $(RADIANT_CMAKE_FLAGS)
RADIANT_CMAKE_FLAGS := $(RADIANT_CMAKE_FLAGS) -DRADIANT_BUILD_PLATFORM=$(RADIANT_BUILD_PLATFORM)

# default commands and such
7ZA=7za
MSBUILD_BINARY='/c/windows/Microsoft.NET/Framework/v4.0.30319/msbuild.exe'
MSBUILD_FLAGS= -nologo -p:PlatformToolset=$(RADIANT_VISUAL_STUDIO_PLATFORM_TOOLS_VERSION) $(RADIANT_MSBUILD_FLAGS)
MSBUILD=$(MSBUILD_BINARY) $(MSBUILD_FLAGS) -maxcpucount
MSBUILD_SERIAL=$(MSBUILD_BINARY) $(MSBUILD_FLAGS) -maxcpucount:1

CMAKE_C_FLAG_OVERRIDE=-DCMAKE_USER_MAKE_RULES_OVERRIDE=${MAKE_ROOT_DOS}/c_flag_overrides.cmake
CMAKE_CXX_FLAG_OVERRIDE=-DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX=${MAKE_ROOT_DOS}/cxx_flag_overrides.cmake
CMAKE_FLAG_OVERRIDE=$(CMAKE_C_FLAG_OVERRIDE) $(CMAKE_CXX_FLAG_OVERRIDE)
CMAKE=cmake.exe -T $(RADIANT_VISUAL_STUDIO_PLATFORM_TOOLS_VERSION) $(CMAKE_FLAG_OVERRIDE) $(RADIANT_CMAKE_FLAGS)

#CMAKE=cmake.exe
VCUPGRADE=vcupgrade -overwrite

