
BUILD_TYPE   ?= debug

ifeq ($(BUILD_TYPE), release)
	MSBUILD_CONFIGURATION=release
endif
ifeq ($(BUILD_TYPE), opt)
	MSBUILD_CONFIGURATION=relwithdebinfo
endif
ifeq ($(BUILD_TYPE), debug)
	MSBUILD_CONFIGURATION=debug
endif

ifeq (_$(MSBUILD_CONFIGURATION)_, __)
   $(error Unknown BUILD_TYPE=$(BUILD_TYPE). Must be debug, opt, or release.)
endif

# default commands and such
7ZA=7za
MSBUILD='/c/windows/Microsoft.NET/Framework/v4.0.30319/msbuild.exe' -nologo -maxcpucount -p:PlatformToolset=v110 

CMAKE_C_FLAG_OVERRIDE=-DCMAKE_USER_MAKE_RULES_OVERRIDE=${MAKE_ROOT_DOS}/c_flag_overrides.cmake
CMAKE_CXX_FLAG_OVERRIDE=-DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX=${MAKE_ROOT_DOS}/cxx_flag_overrides.cmake
CMAKE_FLAG_OVERRIDE=$(CMAKE_C_FLAG_OVERRIDE) $(CMAKE_CXX_FLAG_OVERRIDE)
CMAKE=cmake.exe $(CMAKE_FLAG_OVERRIDE)

#CMAKE=cmake.exe
VCUPGRADE=vcupgrade -overwrite

