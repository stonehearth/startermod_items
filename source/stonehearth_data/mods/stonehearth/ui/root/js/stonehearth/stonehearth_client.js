
$(document).ready(function(){
   $(top).on("radiant_remove_ladder", function (_, e) {
      var item = e.event_data.self

      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:popup'} )
      App.stonehearthClient.removeLadder(e.event_data.self);
   });
});

var StonehearthClient;

(function () {
   StonehearthClient = SimpleClass.extend({

      init: function() {
         var self = this;

         $(document).on('stonehearthReady', function() {
            self._initTools();
         });

         radiant.call('stonehearth:get_client_service', 'build_editor')
            .done(function(e) {
               self._build_editor = e.result;
               radiant.trace(self._build_editor)
                  .progress(function(change) {
                     $(top).trigger('selected_sub_part_changed', change.selected_sub_part);
                  });
            })
            .fail(function(e) {
               console.log('error getting build editor')
               console.dir(e)
            })
      
         radiant.call('stonehearth:get_service', 'build')
            .done(function(e) {
               self._build_service = e.result;
            })
            .fail(function(e) {
               console.log('error getting build service')
               console.dir(e)
            })
      
         $(document).mousemove( function(e) {
            self.mouseX = e.pageX; 
            self.mouseY = e.pageY;
         });

         radiant.call('stonehearth:get_town_name')
            .done(function(e) {
               self.gameState.settlementName = e.townName;
            });
      },

      _initTools: function() {
         this._initBuildLadderTool();
         this._initRemoveLadderTool();
         this._initHarvestTool();
         this._initStockpileTool();
         this._initPlaceItemTool();
         this._initBuildWallTool();
         this._initCreateFarmTool();
         this._initCreateTrappingGrounds();
         this._initBuildFloorTool();
         this._initEraseFloorTool();
         this._initApplyConstructionDataOptionsTool();
         this._initGrowRoofTool();
         this._initGrowWallsTool();
         this._initReplaceStructureTool();
         this._initAddDoodadTool();
      },

      gameState: {
         settlementName: 'Lah Salitos',
         saveKey: null 
      },

      settlementName: function(value) {
         if (value) {
            this.gameState.settlementName = value;
         }
         return this.gameState.settlementName;
      },

      doCommand: function(entity, command) {
         if (!command.enabled) {
            return;
         }
         var event_name = '';

         if (command.action == 'fire_event') {
            // xxx: error checking would be nice!!
            var e = {
               entity : entity,
               event_data : command.event_data
            };
            $(top).trigger(command.event_name, e);
            
            event_name = command.event_name.toString().replace(':','_')

         } else if (command.action == 'call') {
            if (command.object) {
               radiant.call_objv(command.object, command['function'], command.args)            
            } else {
               radiant.callv(command['function'], command.args)
            }
            
            event_name = command['function'].toString().replace(':','_')
            
         } else {
            throw "unknown command.action " + command.action
         }
      },

      deactivateAllTools: function() {
         var self = this;
         return radiant.call('stonehearth:deactivate_all_tools');
      },

      // deprecated. Keeping the code around while StonehearthTool solidifies
      _callTool_DEPRECATED: function(toolFunction, preCall) {
         var self = this;

         var deferred = new $.Deferred();

         var activateTool = function() {
            if (preCall) {
               preCall();
            }
            self._activeTool = toolFunction()
               .done(function(response) {
                  self._activeTool = null;
                  deferred.resolve(response);
               })
               .fail(function(response) {
                  self._activeTool = null;
                  deferred.reject(response);
               });
         };

         if (self._activeTool) {
            // If we have an active tool, trigger a deactivate so that when that
            // tool completes, we'll activate the new tool.
            self._activeTool.always(activateTool);
            this.deactivateAllTools();
         } else {
            activateTool();
         }

         return deferred;
      },

      showTip: function(title, description, options) {
         $(top).trigger('radiant_show_tip', {
            title : title,
            description : description
         });
      },

      hideTip: function() {
         $(top).trigger('radiant_hide_tip');
      },

      _characterSheet: null,
      showCharacterSheet: function(entity) {
         if (this._characterSheet != null && !this._characterSheet.isDestroyed) {
            this._characterSheet.set('uri', entity);
            //this._characterSheet.destroy();
            //this._characterSheet = null;
         } else {
            this._characterSheet = App.gameView.addView(App.StonehearthCitizenCharacterSheetView, { uri: entity });   
         }
      },

      _initPlaceItemTool: function() {
         var self = this;
         this._placeItemTool = new StonehearthTool();

         var tooltipSequence = [
            { 
               title: i18n.t('stonehearth:item_placement_title'),
               description : i18n.t('stonehearth:item_placement_description')
            }
         ];

         var fn = function(item) {
               return radiant.call('stonehearth:choose_place_item_location', item)
                  .done(function(response) {
                     if (response.more_items) {
                        self.placeItem(response.item_uri);
                     }
                  })
            }

         this._placeItemTool.toolFunction(fn)
            .tooltipSequence(tooltipSequence)
            .done(function(response) {
               radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} )
            })
            .always(function(response) {
               App.setGameMode('normal');
            });
      },

      // item is a reference to an actual entity, not a class of entities like stonehearth:comfy_bed
      placeItem: function(item) {
         App.setGameMode('build');
         return this._placeItemTool.run(item)
      },

      _initBuildLadderTool: function() {
         var self = this;
         this._buildLadderTool = new StonehearthTool();

         var tooltipSequence = [
            { 
               title: i18n.t('stonehearth:build_ladder_title'),
               description : 'Right click to exit this mode'
            }
         ];

         var fn = function() {
               return radiant.call_obj(self._build_editor, 'build_ladder');
            }

         this._buildLadderTool.toolFunction(fn)
            .tooltipSequence(tooltipSequence)
            .loops(true)
            .done(function(response) {
               radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} )
            })
            .fail(function(response) {
               App.setGameMode('normal');
            });
      },
      
      buildLadder: function() {
         App.setGameMode('build');
         return this._buildLadderTool.run()
      },

      _initRemoveLadderTool: function() {
         var self = this;
         this._removeLadderTool = new StonehearthTool();


         var fn = function(ladder) {
               return radiant.call_obj(self._build_service, 'remove_ladder_command', ladder);
            }

         this._removeLadderTool.toolFunction(fn);
      },

      
      removeLadder: function(ladder) {
         return this._removeLadderTool.run(ladder)
      },

      _initHarvestTool: function() {
         this._harvestTool = new StonehearthTool();

         var tooltipSequence = [
            { 
               title: 'Click and drag to harvest resources',
               description : 'Right click to exit this mode'
            }
         ];

         var fn = function() {
               return radiant.call('stonehearth:box_harvest_resources');
            }

         this._harvestTool.toolFunction(fn)
            .tooltipSequence(tooltipSequence)
            .progress(function(response) {
               radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:popup'} );
            })
            .loops(true);
      },

      boxHarvestResources: function() {
         return this._harvestTool.run()
      },

      _initStockpileTool: function() {
         this._stockpileTool = new StonehearthTool();

         var tooltipSequence = [
            { 
               title: 'Click and drag to make a stockpile',
               description : 'Right click to exit this mode'
            }
         ];

         var fn = function() {
               return radiant.call('stonehearth:choose_stockpile_location');
            }

         this._stockpileTool.toolFunction(fn)
            .tooltipSequence(tooltipSequence)
            .progress(function(response) {
               radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
               radiant.call('stonehearth:select_entity', response.stockpile);
            })
            .loops(true);
      },

      createStockpile: function() {
         App.setGameMode('zones');
         return this._stockpileTool.run();
      },

      _initCreateFarmTool: function() {
         var self = this;
         this._createFarmTool = new StonehearthTool();

         var tooltipSequence = [
            { 
               title: 'Click and drag to designate a farm.',
               description : 'Farmers will plant in field zones.'
            }
         ];

         var fn = function(item) {
            return radiant.call('stonehearth:choose_new_field_location')
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
                  radiant.call('stonehearth:select_entity', response.field);
               })
            }

         this._createFarmTool.toolFunction(fn)
            .tooltipSequence(tooltipSequence)
            .loops(true)
            .always(function(response) {
               App.setGameMode('normal');
            });
      },

      createFarm: function(o) {
         App.setGameMode('zones');
         return this._createFarmTool.run();
      },

      _initCreateTrappingGrounds: function() {
         var self = this;
         this._createTrappingGroundsTool = new StonehearthTool();

         var tooltipSequence = [
            { 
               title: 'Click and drag to designate a trapping zone',
               description : 'Trappers will hunt in trapping zones'
            }
         ];

         var fn = function(item) {
            return radiant.call('stonehearth:choose_trapping_grounds_location')
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
                  radiant.call('stonehearth:select_entity', response.trapping_grounds);
               })
            }

         this._createTrappingGroundsTool.toolFunction(fn)
            .tooltipSequence(tooltipSequence)
            .loops(true)
            .always(function(response) {
               App.setGameMode('normal');
            });
      },

      createTrappingGrounds: function(o) {
         App.setGameMode('zones');
         return this._createTrappingGroundsTool.run();
      },

      undo: function () {
         radiant.call_obj(this._build_service, 'undo_command')
      },

      _initBuildWallTool: function() {
         var self = this;

         this._buildWallTool = new StonehearthTool();

         var tooltipSequence = [
            { 
               title: 'Click to build wall segments',
               description : 'Hold down SHIFT while clicking to draw connected walls'
            }
         ];

         var fn = function(column, wall) {
               return radiant.call_obj(self._build_editor, 'place_new_wall', column, wall);
            }

         this._buildWallTool
            .toolFunction(fn)
            .tooltipSequence(tooltipSequence)
            .sound('stonehearth:sounds:ui:start_menu:popup')
            .loops(true)
            .progress(function(response) {
               radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
            });
      },

      buildWall: function(column, wall, precall) {
         return this._buildWallTool      
            .preCall(precall)
            .run(column, wall)
      },

      _initBuildFloorTool: function() {
         var self = this;

         this._buildFloorTool = new StonehearthTool();

         var tooltipSequence = [
            { 
               title: 'Click and drag to build floor',
               description : 'Right click to exit this mode'
            }
         ];

         var fn = function(floorBrush) {
               return radiant.call_obj(self._build_editor, 'place_new_floor', floorBrush) 
            }

         this._buildFloorTool.toolFunction(fn)
            .tooltipSequence(tooltipSequence)
            .sound('stonehearth:sounds:ui:start_menu:popup')
            .loops(true)
            .progress(function(response) {
               radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
            });
      },

      buildFloor: function(floorBrush, precall) {
         return this._buildFloorTool
            .preCall(precall)
            .run(floorBrush)
      },

      _initEraseFloorTool: function() {
         var self = this;

         this._eraseFloorTool = new StonehearthTool();

         var tooltipSequence = [
            { 
               title: 'Click and drag to erase floor',
               description : 'Right click to exit this mode'
            }
         ];

         var fn = function(floorBrush) {
               return radiant.call_obj(self._build_editor, 'erase_floor')
            }

         this._eraseFloorTool.toolFunction(fn)
            .tooltipSequence(tooltipSequence)
            .sound('stonehearth:sounds:ui:start_menu:popup')
            .loops(true)
            .progress(function(response) {
               radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
            });
      },

      eraseFloor: function(precall) {
         return this._eraseFloorTool
            .preCall(precall)
            .run()
      },


      _initGrowRoofTool: function() {
         var self = this;

         this._growRoofTool = new StonehearthTool();

         var fn = function(roof) {
               return radiant.call_obj(self._build_editor, 'grow_roof', roof)
            }

         this._growRoofTool.toolFunction(fn)
            .sound('stonehearth:sounds:place_structure')
            .loops(true);
      },

      growRoof: function(roof, precall) {
         return this._growRoofTool
            .preCall(precall)
            .run(roof)
      },

      _initApplyConstructionDataOptionsTool: function() {
         var self = this;

         this._applyConstructionDataOptionsTool = new StonehearthTool();

         var fn = function(blueprint, options) {
               return radiant.call_obj(self._build_service, 'apply_options_command', blueprint, options);
            }

         this._applyConstructionDataOptionsTool.toolFunction(fn);
      },

      applyConstructionDataOptions: function(blueprint, options) {
         return this._applyConstructionDataOptionsTool.run(blueprint.__self, options);
      },

      instabuild: function(building) {
         return radiant.call_obj(this._build_service, 'instabuild_command', building.__self);
      },

      setGrowRoofOptions: function(options) {
         return radiant.call_obj(this._build_editor, 'set_grow_roof_options', options);
      },


      _initGrowWallsTool: function() {
         var self = this;
         this._growWallsTool = new StonehearthTool();

         var tooltipSequence = [
            'Grow walls around a floor by clicking on it',
         ];

         var fn = function(column, wall) {
               return radiant.call_obj(self._build_editor, 'grow_walls', column, wall)
            }

         this._growWallsTool.toolFunction(fn)
            .tooltipSequence(tooltipSequence)
            .loops(true)
            .progress(function(response) {
               radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} )
            });
      },

      growWalls: function(column, wall, precall) {
         return this._growWallsTool
            .preCall(precall)
            .run(column, wall)
      },

      _initReplaceStructureTool: function() {
         var self = this;

         this._replaceStructureTool = new StonehearthTool();

         var fn = function(old_structure, new_structure_uri) {
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
            return radiant.call_obj(self._build_service, 'substitute_blueprint_command', old_structure.__self, new_structure_uri)
               .done(function(o) {
                  if (o.new_selection) {
                     radiant.call('stonehearth:select_entity', o.new_selection);
                  }
               });
            }

         this._replaceStructureTool.toolFunction(fn);
      },

      replaceStructure: function(old_structure, new_structure_uri) {
         if (old_structure) {
            return this._replaceStructureTool.run(old_structure, new_structure_uri)   
         }
      },

      _initAddDoodadTool: function() {
         var self = this;

         this._addDoodadTool = new StonehearthTool();

         var tooltipSequence = [
            'Click on walls or floors to place the item',
         ];


         var fn = function(doodadUri) {
               return radiant.call_obj(self._build_editor, 'add_doodad', doodadUri)
                  .done(function(response) {
                     radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
                  });
            }

         this._addDoodadTool.toolFunction(fn)
            .tooltipSequence(tooltipSequence)
            .loops(true);
      },

      addDoodad: function(doodadUri, precall) {
         return this._addDoodadTool
            .preCall(precall)
            .run(doodadUri)
      },

      _townMenu: null,
      showTownMenu: function(show) {
         // hide the other population managers....oh lord this is ugly code
         if (this._citizenManager) {
            this._citizenManager.destroy();
            this._citizenManager = null;
         }

         if (this._crafterManager) {
            this._crafterManager.destroy();
            this._crafterManager = null;
         }

         // toggle the town menu
         if (!this._townMenu || this._townMenu.isDestroyed) {
            this._townMenu = App.gameView.addView(App.StonehearthTownView);
         } else {
            this._townMenu.destroy();
            this._townMenu = null;
         }         
      },

      _citizenManager: null,
      showCitizenManager: function(show) {
         // hide the other population managers....oh lord this is ugly code
         if (this._crafterManager) {
            this._crafterManager.destroy();
            this._crafterManager = null;
         }

         if (this._townMenu) {
            this._townMenu.destroy();
            this._townMenu = null;
         }

         // toggle the citizenManager
         if (!this._citizenManager || this._citizenManager.isDestroyed) {
            this._citizenManager = App.gameView.addView(App.StonehearthCitizensView);
         } else {
            this._citizenManager.destroy();
            this._citizenManager = null;
         }
      },

      _crafterManager: null,
      showCrafterManager: function(show) {
         // hide the other population managers....oh lord this is ugly code
         if (this._citizenManager) {
            this._citizenManager.destroy();
            this._citizenManager = null;
         }

         if (this._townMenu) {
            this._townMenu.destroy();
            this._townMenu = null;
         }         
         
         // toggle the citizenManager
         if (!this._crafterManager || this._crafterManager.isDestroyed) {
            this._crafterManager = App.gameView.addView(App.StonehearthCraftersView);
         } else {
            this._crafterManager.destroy();
            this._crafterManager = null;
         }
      },

      _redAlertWidget: null,
      rallyWorkers: function() {
         radiant.call('stonehearth:worker_combat_enabled')
            .done(function(response) {
               if (response.enabled) {
                  radiant.call('stonehearth:disable_worker_combat');
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:scenarios:redalert_off'});
                  if (self._redAlertWidget) {
                     self._redAlertWidget.destroy();   
                  }
               } else {
                  radiant.call('stonehearth:enable_worker_combat');
                  self._redAlertWidget = App.gameView.addView(App.StonehearthRedAlertWidget);
               }
            })
      },

   });
   App.stonehearthClient = new StonehearthClient();
})();


