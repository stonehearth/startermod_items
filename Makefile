include make/settings.mk

STONEHEARTH_ROOT = ${CURDIR}
MAKE_ROOT        = $(STONEHEARTH_ROOT)/make
BUILD_ROOT       = $(STONEHEARTH_ROOT)/build

.PHONY: default
default: configure submodules stonehearth

# TODO : put rm in the devroot bin dir
clean:
	rmdir /q/s build

.PHONY: submodules
submodules:
	MAKE_ROOT=$(MAKE_ROOT) $(MAKE_ROOT)/build-submodules.py

.PHONY: configure
configure:
	cmd.exe /c "cmake.exe -H. -Bbuild -G\"Visual Studio 11\""

.PHONY: stonehearth
stonehearth:
	$(MSBUILD) $(BUILD_ROOT)/Stonehearth.sln -p:configuration=debug -t:protocols
	$(MSBUILD) $(BUILD_ROOT)/Stonehearth.sln -p:configuration=debug

ide: configure
	start build/Stonehearth.sln

.PHONY: dependency-graph
dependency-graph:
	cmake -H. -Bbuild -G"Visual Studio 11" --graphviz=deps.dot
