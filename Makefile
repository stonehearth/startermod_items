-include local.mk
include make/settings.mk

STONEHEARTH_ROOT   = ${CURDIR}
MAKE_ROOT          = $(STONEHEARTH_ROOT)/make
MAKE_ROOT_DOS      = $(shell pwd -W)/make
BUILD_ROOT         = $(STONEHEARTH_ROOT)/build
SCRIPTS_ROOT       = $(STONEHEARTH_ROOT)/scripts
DEPLOYMENT_ROOT    = $(BUILD_ROOT)/deployment

.PHONY: default
default: submodules configure stonehearth

.PHONY: clean
clean:
	rm -rf build

.PHONY: official-build
official-build: init-build submodules configure stonehearth deployment


.PHONY: init-build
init-build:
	-mkdir -p build
	-rm build/build_overrides.h
	$(MAKE_ROOT)/init_build_number.py > build/build_overrides.h

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
	cd source/stonehearth_data && ../../build/source/client_app/$(MSBUILD_CONFIGURATION)/Stonehearth.exe --game.script=stonehearth_tests/$*_test.lua&

run:
	cd source/stonehearth_data && ../../build/source/client_app/$(MSBUILD_CONFIGURATION)/Stonehearth.exe 

# make a decoda project!
.PHONY: decoda-project
decoda-project:
	scripts/make_decoda_project.py stonehearth.deproj source/stonehearth_data/mods

.PHONY: dependency-graph
dependency-graph:
	cmake -H. -Bbuild -G"Visual Studio 11" --graphviz=deps.dot

.PHONY: deployment
deployment: stonehearth
	sh $(SCRIPTS_ROOT)/copy-deployment-files.sh $(DEPLOYMENT_ROOT) $(BUILD_ROOT)/source/client_app/$(MSBUILD_CONFIGURATION) $(STONEHEARTH_ROOT)/source/stonehearth_data
