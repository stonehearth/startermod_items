include make/settings.mk

STONEHEARTH_ROOT   = ${CURDIR}
MAKE_ROOT          = $(STONEHEARTH_ROOT)/make
MAKE_ROOT_DOS      = $(shell pwd -W)/make
BUILD_ROOT         = $(STONEHEARTH_ROOT)/build
SCRIPTS_ROOT       = $(STONEHEARTH_ROOT)/scripts

# xxx: it would be *great* if we could specify an absolute or relative
# path for things like STAGE_ROOT and things would "jsut work".  Doing
# so requires 'readlink' in our scripts to convert things to absolute.
# The MSYS that ships with Git does not have readlink (!).  We can fix
# this by using a more recent version of MSYS and pulling git into that
# but that's a lot of work.  This, incidentally, would also get us a
# version of gmake from 2006 instead of 2000 (!!)
STAGE_ROOT         = build/stage/stonehearth
ZIP_PACKAGE_ROOT   = build/game-package
STEAM_PACKAGE_ROOT = build/steam-package

# figure out where to find the data files for the 'make run*' commands
ifeq ($(RUN_STAGED),)
  RUN_ROOT=$(STONEHEARTH_ROOT)/source/stonehearth_data
else
  RUN_ROOT=$(STAGE_ROOT)
endif

.PHONY: default
default: submodules configure crash_reporter stonehearth

.PHONY: clean
clean:
	rm -rf build

.PHONY: official-build
official-build: clean init-build submodules configure crash_reporter stonehearth symbols stage game-package steam-package

.PHONY: init-build
init-build:
	-mkdir -p build
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
	-rm build/CMakeCache.txt
	cmd.exe /c "$(CMAKE) -H. -Bbuild -G\"Visual Studio 11\""

.PHONY: stonehearth
stonehearth:
	@echo Build type is ${BUILD_TYPE}.
	$(MSBUILD) $(BUILD_ROOT)/Stonehearth.sln -p:configuration=$(MSBUILD_CONFIGURATION) -t:protobuf
	$(MSBUILD) $(BUILD_ROOT)/Stonehearth.sln -p:configuration=$(MSBUILD_CONFIGURATION) -t:stonehearth

.PHONY: crash_reporter
crash_reporter:
	@echo Build type is ${BUILD_TYPE}.
	$(MSBUILD) $(BUILD_ROOT)/Stonehearth.sln -p:configuration=$(MSBUILD_CONFIGURATION) -t:protobuf
	$(MSBUILD) $(BUILD_ROOT)/Stonehearth.sln -p:configuration=$(MSBUILD_CONFIGURATION) -t:crash_reporter_server

.PHONY: symbols
symbols:
	modules/breakpad/package/src/tools/windows/dump_syms/Release/dump_syms.exe \
	  $(BUILD_ROOT)/source/stonehearth/$(MSBUILD_CONFIGURATION)/stonehearth.exe > \
	  $(BUILD_ROOT)/source/stonehearth/$(MSBUILD_CONFIGURATION)/Stonehearth.sym

.PHONY: ide
ide: configure
	start build/Stonehearth.sln

run-%-test:
	cd $(RUN_ROOT) && ../../build/source/stonehearth/$(MSBUILD_CONFIGURATION)/Stonehearth.exe --game.script=stonehearth_tests/$*_test.lua&

run:
	cd $(RUN_ROOT) && ../../build/source/stonehearth/$(MSBUILD_CONFIGURATION)/Stonehearth.exe&

# make a decoda project!
.PHONY: decoda-project
decoda-project:
	scripts/make_decoda_project.py stonehearth.deproj source/stonehearth_data/mods

.PHONY: dependency-graph
dependency-graph:
	cmake -H. -Bbuild -G"Visual Studio 11" --graphviz=deps.dot

.PHONY: stage
stage:
	sh $(SCRIPTS_ROOT)/stage/stage_stonehearth.sh -o $(STAGE_ROOT) -t $(MSBUILD_CONFIGURATION) -c -a

.PHONY: perf-stage
perf-stage:
	sh $(SCRIPTS_ROOT)/stage/stage_stonehearth.sh -o $(STAGE_ROOT) -t $(MSBUILD_CONFIGURATION) -c -a -m perf_test

.PHONY: perf-run
perf-run:
	curl -F args=$(ARGS) -F settings=@$(TEST_CONFIG) -F file=@$(ZIP_PACKAGE_ROOT)/stonehearth-game.zip -K slaveurls.txt	

.PHONY: perfexp
perfexp: perf-stage game-package perf-run

.PHONY: game-package
game-package:
	echo 'creating game package'
	-mkdir -p $(ZIP_PACKAGE_ROOT)
	-rm $(ZIP_PACKAGE_ROOT)/stonehearth-game.zip
	cd $(STAGE_ROOT)/.. && $(7ZA) a -bd -r -tzip $(STONEHEARTH_ROOT)/$(ZIP_PACKAGE_ROOT)/stonehearth-game.zip *

.PHONY: steam-package
steam-package:
	echo 'creating steam package'
	-mkdir -p $(STEAM_PACKAGE_ROOT)
	-rm $(STEAM_PACKAGE_ROOT)/stonehearth-steam.zip
	cd $(STONEHEARTH_ROOT)/scripts/steampipe && $(7ZA) a -bd -r -tzip -mx=9 $(STONEHEARTH_ROOT)/$(STEAM_PACKAGE_ROOT)/stonehearth-steam.zip *

.PHONY: docs
docs:
	cd $(STONEHEARTH_ROOT)/modules/stonehearth-editor && $ lua/dist/bin/lua.exe shed.lua document $(STONEHEARTH_ROOT)/source/stonehearth_data/mods/stonehearth/docs/ --doc_output_dir=$(BUILD_ROOT)/docs