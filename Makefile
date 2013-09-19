-include local.mk
include make/settings.mk

STONEHEARTH_ROOT = ${CURDIR}
MAKE_ROOT        = $(STONEHEARTH_ROOT)/make
MAKE_ROOT_DOS    = $(shell pwd -W)/make
BUILD_ROOT       = $(STONEHEARTH_ROOT)/build

.PHONY: default
default: submodules configure stonehearth

clean:
	rm -rf build

.PHONY: submodules
submodules:
	git submodule init
	git submodule update --remote
	$(MAKE_ROOT)/build-submodules.py

%-module:
	MAKE_ROOT=$(MAKE_ROOT) MAKE_ROOT_DOS=$(MAKE_ROOT_DOS) make -C modules/$*

.PHONY: configure
configure:
	cmd.exe /c "$(CMAKE) -H. -Bbuild -G\"Visual Studio 11\""

.PHONY: stonehearth
stonehearth:
	@echo Build type is ${BUILD_TYPE}.
	$(MSBUILD) $(BUILD_ROOT)/Stonehearth.sln -p:configuration=$(MSBUILD_CONFIGURATION) -t:protocols
	$(MSBUILD) $(BUILD_ROOT)/Stonehearth.sln -p:configuration=$(MSBUILD_CONFIGURATION)

.PHONY: ide
ide: configure
	start build/Stonehearth.sln

run-%-test:
	cd source/stonehearth_data && ../../build/source/client_app/$(MSBUILD_CONFIGURATION)/client_app.exe --game.script=stonehearth_tests/$*_test.lua&

run:
	cd source/stonehearth_data && ../../build/source/client_app/$(MSBUILD_CONFIGURATION)/client_app.exe 

.PHONY: dependency-graph
dependency-graph:
	cmake -H. -Bbuild -G"Visual Studio 11" --graphviz=deps.dot
