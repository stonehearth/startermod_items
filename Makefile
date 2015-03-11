include make/settings.mk

STONEHEARTH_ROOT   = ${CURDIR}
MAKE_ROOT          = $(STONEHEARTH_ROOT)/make
MAKE_ROOT_DOS      = $(shell pwd -W)/make
BUILD_DIR          = build/$(RADIANT_BUILD_PLATFORM)
BUILD_ROOT         = $(STONEHEARTH_ROOT)/$(BUILD_DIR)
SCRIPTS_ROOT       = $(STONEHEARTH_ROOT)/scripts

STAGE_FLAGS        = -t $(MSBUILD_CONFIGURATION) -x $(RADIANT_BUILD_PLATFORM)
STAGE              = sh $(SCRIPTS_ROOT)/stage/stage_stonehearth.sh $(STAGE_FLAGS)

# xxx: it would be *great* if we could specify an absolute or relative
# path for things like STAGE_ROOT and things would "jsut work".  Doing
# so requires 'readlink' in our scripts to convert things to absolute.
# The MSYS that ships with Git does not have readlink (!).  We can fix
# this by using a more recent version of MSYS and pulling git into that
# but that's a lot of work.  This, incidentally, would also get us a
# version of gmake from 2006 instead of 2000 (!!)
STAGE_ROOT         = build/stage/stonehearth
STEAM_PACKAGE_ROOT = build/steam-package
ZIP_PACKAGE_ROOT   = build/game-package
TEST_STAGE_ROOT    = $(BUILD_DIR)/test-stage/stonehearth
TEST_PACKAGE_ROOT  = $(BUILD_DIR)/test-package

# figure out where to find the data files for the 'make run*' commands
ifeq ($(RUN_STAGED),)
  RUN_ROOT=$(STONEHEARTH_ROOT)/source/stonehearth_data
  STONEHEARTH_APP = ../../$(BUILD_DIR)/source/stonehearth/$(MSBUILD_CONFIGURATION)/Stonehearth.exe $(SHFLAGS)
else
  RUN_ROOT=$(STAGE_ROOT)
  STONEHEARTH_APP = ../../source/stonehearth/$(MSBUILD_CONFIGURATION)/Stonehearth.exe $(SHFLAGS)
endif

.PHONY: default
default: submodules configure crash_reporter stonehearth

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: clean-all
clean-all:
	rm -rf build

# the legacy official-build target.  does nothing!
.PHONY: official-build
official-build: clean init-build submodules configure crash_reporter stonehearth symbols stage game-package steam-package

# the current official targets.  builds are done in this order with these shells:
#
#    official-build-setup 	 (32-bit shell)
#    official-build-x64   	 (64-bit shell)
#    official-build-x86  	 (32-bit shell)
#    official-build-package (32-bit shell)
#
.PHONY: official-build-setup
official-build-setup: clean-all

.PHONY: official-build-x86
official-build-x86: official-build-platform

.PHONY: official-build-x64
official-build-x64: official-build-platform

official-build-platform: submodules init-build configure crash_reporter stonehearth symbols

.PHONY: official-build-package
official-build-package: stage game-package steam-package

.PHONY: git-tag
git-tag:
	git tag -f -a "$(BAMBOO_BRANCH_NAME)_$(BAMBOO_BUILD_NUMBER)" $(BAMBOO_BRANCH_REVISION)
	git push origin "$(BAMBOO_BRANCH_NAME)_$(BAMBOO_BUILD_NUMBER)"
#	git remote add central $(BAMBOO_RCS_SERVER)
#	git push central ${bamboo.buildNumber}
#	git ls-remote --exit-code --tags central ${bamboo.buildNumber} 

# fake-official-build-x86 and fake-official-build-x64 for testing in a developer environment
fake-official-build-%:
	BAMBOO_BRANCH_NAME=branch BAMBOO_BUILD_TIME=now BAMBOO_PRODUCT_VERSION_MAJOR=1 BAMBOO_PRODUCT_VERSION_PATCH=1 BAMBOO_PRODUCT_VERSION_MINOR=1 BAMBOO_BUILD_NUMBER=1111 BAMBOO_PRODUCT_NAME=Stonehearth make BAMBOO_PRODUCT_IDENTIFIER=sh BAMBOO_PRODUCT_IDENTIFIER=stonehearth BAMBOO_BRANCH_REVISION=efgh official-build-$*

.PHONY: init-build
init-build:
	-mkdir -p $(BUILD_ROOT)
	$(MAKE_ROOT)/init_build_number.py > $(BUILD_ROOT)/build_overrides.h

.PHONY: submodules
submodules:
	git submodule init
	git submodule update --remote	
	$(MAKE_ROOT)/build-submodules.py

%-module:
	MAKE_ROOT=$(MAKE_ROOT) MAKE_ROOT_DOS=$(MAKE_ROOT_DOS) make -C modules/$*

.PHONY: configure
configure:
	cp $(SCRIPTS_ROOT)/git_hooks/pre-push $(STONEHEARTH_ROOT)/.git/hooks/
	-rm $(BUILD_DIR)/CMakeCache.txt
	cmd.exe /c "$(CMAKE) -H. -B$(BUILD_DIR) -G\"$(RADIANT_CMAKE_GENERATOR)\""

.PHONY: stonehearth
stonehearth:
	@echo Build type is ${BUILD_TYPE}.
	$(MSBUILD) $(BUILD_ROOT)/Stonehearth.sln -p:configuration=$(MSBUILD_CONFIGURATION) -t:protobuf
	$(MSBUILD) $(BUILD_ROOT)/Stonehearth.sln -p:configuration=$(MSBUILD_CONFIGURATION) -t:stonehearth
	$(STAGE) -o ../../$(BUILD_DIR)/source/stonehearth/$(MSBUILD_CONFIGURATION) -b

.PHONY: crash_reporter
crash_reporter:
	@echo Build type is ${BUILD_TYPE}.
	$(MSBUILD) $(BUILD_ROOT)/Stonehearth.sln -p:configuration=$(MSBUILD_CONFIGURATION) -t:protobuf
	$(MSBUILD) $(BUILD_ROOT)/Stonehearth.sln -p:configuration=$(MSBUILD_CONFIGURATION) -t:crash_reporter_server

.PHONY: symbols
symbols:
	modules/breakpad/build/$(RADIANT_BUILD_PLATFORM)/release/dump_syms.exe \
	  $(BUILD_ROOT)/source/stonehearth/$(MSBUILD_CONFIGURATION)/lua-5.1.5.jit.dll > \
	  $(BUILD_ROOT)/source/stonehearth/$(MSBUILD_CONFIGURATION)/lua-5.1.5.jit.sym
	modules/breakpad/build/$(RADIANT_BUILD_PLATFORM)/release/dump_syms.exe \
	  $(BUILD_ROOT)/source/stonehearth/$(MSBUILD_CONFIGURATION)/stonehearth.exe > \
	  $(BUILD_ROOT)/source/stonehearth/$(MSBUILD_CONFIGURATION)/Stonehearth.sym

# the cef-symbols target requires updating radbot@support-stage's .authorized_keys
.PHONY: cef-symbols
cef-symbols:
	-rm $(BUILD_DIR)/libcef.sym
	modules/breakpad/build/$(RADIANT_BUILD_PLATFORM)/release/dump_syms.exe \
		$(STONEHEARTH_ROOT)/modules/chromium-embedded/package/cef_binary_3.2171.1902_windows64/Release/libcef.dll > \
		$(BUILD_DIR)/libcef.sym
	scp $(BUILD_DIR)/libcef.sym radbot@support-stage:/home/radbot/libcef.sym
	ssh radbot@support-stage "./publish_symbols.sh libcef"

	-rm $(BUILD_DIR)/libcef.sym
	modules/breakpad/build/$(RADIANT_BUILD_PLATFORM)/release/dump_syms.exe \
		$(STONEHEARTH_ROOT)/modules/chromium-embedded/package/cef_binary_3.2171.1902_windows32/Release/libcef.dll > \
		$(BUILD_DIR)/libcef.sym
	scp $(BUILD_DIR)/libcef.sym radbot@support-stage:/home/radbot/libcef.sym
	ssh radbot@support-stage "./publish_symbols.sh libcef"

.PHONY: ide
ide: configure ide-only

.PHONY: ide-only
ide-only:
	start $(BUILD_DIR)/Stonehearth.sln

.PHONY: run-autotest
run-autotest:
	scripts/test/run_autotest.sh

.PHONY: run-pathfinder-benchmark
run-pathfinder-benchmark:
	cd $(RUN_ROOT) && $(STONEHEARTH_APP) $(FLAGS) --game.main_mod=stonehearth_autotest --mods.stonehearth_autotest.options.script="stonehearth_autotest/tests/pathfinder/pathfinder_benchmark.lua"
	tail $(RUN_ROOT)/stonehearth.log | egrep "solved.*PPS"

run-%-test:
	cd $(RUN_ROOT) && $(STONEHEARTH_APP) $(FLAGS) --game.main_mod=stonehearth_tests --mods.stonehearth_tests.test=$*_test&

run:
	cd $(RUN_ROOT) && $(STONEHEARTH_APP) $(FLAGS) &

.PHONY: run-all-tests-remote
run-all-tests-remote:
	scripts/test/run_remote_autotest.py

 
.PHONY: run-all-perf-tests-remote
run-all-perf-tests-remote:
	scripts/test/run_remote_autotest.py -p -g perf_all
	curl -F results=@$(BUILD_DIR)/combined_results.shperf.json http://10.1.10.51:8087/_result?build_number=$(BAMBOO_BUILD_NUMBER)

# Collect performance data from a local build.  Invokes like:
# make run-perf-exp SCRIPT=perf_camera_orbit.lua FUNC=orbit CONFIG=ultra_quality DESC=description_goes_here
.PHONY: run-perf-exp
run-perf-exp:
	scripts/test/run_remote_autotest.py -p -s performance/$(SCRIPT) -f $(FUNC) -c $(CONFIG)
	curl -F results=@$(BUILD_DIR)/combined_results.shperf.json http://build-win-01:8087/_experiment?description=$(DESC)

.PHONY: perf-exp
perf-exp: test-stage test-package run-perf-exp

# make a decoda project!
.PHONY: decoda-project
decoda-project:
	scripts/make_decoda_project.py stonehearth.deproj source/stonehearth_data/mods

.PHONY: dependency-graph
dependency-graph:
	cmake -H. -B$(BUILD_DIR) -G"Visual Studio 11" --graphviz=deps.dot

.PHONY: stage
stage:
	$(STAGE) -o $(STAGE_ROOT)     -x x86 -c -b -s -d
	$(STAGE) -o $(STAGE_ROOT)/x64 -x x64 -c -b -s

.PHONY: test-stage
test-stage:
	$(STAGE) -o $(TEST_STAGE_ROOT) -c -a -m stonehearth_autotest -m autotest_framework

.PHONY: test-package
test-package:
	echo 'creating test package'
	-mkdir -p $(TEST_PACKAGE_ROOT)
	-rm $(TEST_PACKAGE_ROOT)/stonehearth-test.zip
	cp $(STONEHEARTH_ROOT)/radsub.py $(TEST_STAGE_ROOT)/radsub.py
	cd $(TEST_STAGE_ROOT)/.. && $(7ZA) a -bd -r -tzip $(STONEHEARTH_ROOT)/$(TEST_PACKAGE_ROOT)/stonehearth-test.zip *

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

.PHONY: update-templates
update-templates:
	cd $(STONEHEARTH_ROOT)/source/stonehearth_data/ && $(SCRIPTS_ROOT)/update_templates.py

