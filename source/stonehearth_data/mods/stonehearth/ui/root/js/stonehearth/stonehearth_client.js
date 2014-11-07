
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
      
         radiant.call('stonehearth:get_client_service', 'subterranean_view')
            .done(function(e) {
               self._subterranean_view = e.result;
            })
            .fail(function(e) {
               console.log('error getting subterranean_view client service')
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

      getActiveTool: function() {
         return this._activeTool;
      },

      deactivateAllTools: function() {
         var self = this;
         console.log('deactivating all tools');
         return radiant.call('stonehearth:deactivate_all_tools')
            .always(function() {
               self._activeTool = null;
            });
      },

      // Wrapper to call all tools, handling the boilerplate tool management.
      _callTool: function(toolName, toolFunction, preCall) {
         var self = this;
         var deferred = new $.Deferred();

         var debug_log = function(str) {
             // console.log('(processing tool ' + toolName + ') ' + str);
         };

         debug_log('new call to _callTool... ');

         var activateTool = function() {
            debug_log('activating tool...')
            if (preCall) {
               debug_log('in preCall for activateTool...');
               preCall();
            }
            debug_log('calling tool function...');            
            self._activeTool = toolFunction()
               .done(function(response) {
                  debug_log('clearing self._activeTool in tool done handler...');
                  self._activeTool = null;
                  deferred.resolve(response);
               })
               .fail(function(response) {
                  debug_log('clearing self._activeTool in tool fail handler...');
                  self._activeTool = null;
                  deferred.reject(response);
               });
         };

         if (self._activeTool) {
            // If we have an active tool, trigger a deactivate so that when that
            // tool completes, we'll activate the new tool.
            debug_log('installing activateTool always handler on old tool to activate this one (crazy!)');
            self._activeTool.always(activateTool);
            this.deactivateAllTools();
         } else {
            debug_log('activating tool immediately, since there is no active one now.');
            activateTool();
         }
         debug_log('returning deferred from _callTool');

         return deferred;
      },

      _currentTip: null,
      showTip: function(title, description, options) {
         var self = this
         var o = options || {};

         if (o.i18n) {
            title = i18n.t(title);
            description = i18n.t(description);
         }

         if (self._currentTip && self._currentTip.title == title && self._currentTip.description == description) {
            // do nothing         
         } else {
            self._destroyCurrentTip();   
            self._currentTip = App.gameView.addView(App.StonehearthTipPopup, 
               { 
                  title: title,
                  description: description
               });
         }

         return self._currentTip;
      },

      // if tip is null, whatever tip is showing will be unconditionally hidden
      hideTip: function(tip) {
         if (tip) {
            if (this._currentTip == tip) {
               this._destroyCurrentTip();
            }
         } else {
            this._destroyCurrentTip();
         }
      },

      _destroyCurrentTip: function() {
         if (this._currentTip) {
            this._currentTip.destroy();
            this._currentTip = null;
         }
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

      // item is a reference to an actual entity, not a class of entities like stonehearth:furniture:comfy_bed
      placeItem: function(item) {
         var self = this;

         self.showTip('stonehearth:item_placement_title', 'stonehearth:item_placement_description',
            { i18n: true });

         App.setGameMode('build');
         return this._callTool('placeItem', function() {
            return radiant.call('stonehearth:choose_place_item_location', item)
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} )
               })
               .always(function(response) {
                  App.setGameMode('normal');
                  self.hideTip();
               });
         });
      },

      // item type is a uri, not an item entity
      placeItemType: function(itemType) {
         var self = this;

         self.showTip('stonehearth:item_placement_title', 'stonehearth:item_placement_description',
            {i18n: true});

         App.setGameMode('build');
         return this._callTool('placeItemType', function() {
            return radiant.call('stonehearth:choose_place_item_location', itemType)
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} )
                  if (response.more_items) {
                     self.placeItemType(itemType);
                  } else {
                     self.hideTip();
                  }
               })
               .fail(function(response) {
                  //App.setGameMode('normal');
                  self.hideTip();
               });
         });
      },

      // item is a reference to an actual entity, not a class of entities like stonehearth:furniture:comfy_bed
      undeployItem: function(item) {
         var self = this;

         return this._callTool('undeployItem', function() {
            return radiant.call('stonehearth:undeploy_item', item);
         });
      },

      subterraneanSetClip: function(enabled) {
         return radiant.call_obj(this._subterranean_view, 'set_clip_enabled_command', enabled);
      },

      subterraneanMoveUp: function() {
         var self = this;
         radiant.call_obj(self._subterranean_view, 'set_clip_enabled_command', true)
            .done(function(response) {
               if (response) {
                  radiant.call_obj(self._subterranean_view, 'move_clip_height_up_command');
               }
            })
      },

      subterraneanMoveDown: function() {
         var self = this;
         radiant.call_obj(self._subterranean_view, 'set_clip_enabled_command', true)
            .done(function(response) {
               if (response) {
                  radiant.call_obj(self._subterranean_view, 'move_clip_height_down_command');
               }
            })
      },

      // item type is a uri, not an item entity
      buildLadder: function() {
         var self = this;

         var tip = self.showTip('stonehearth:build_ladder_title', 'stonehearth:build_ladder_description',
            {i18n: true});

         App.setGameMode('build');
         return this._callTool('buildLadder', function() {
            return radiant.call_obj(self._build_editor, 'build_ladder')
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
                  self.buildLadder();
               })
               .fail(function(response) {
                  self.hideTip(tip);
               });
         });
      },

      // item type is a uri, not an item entity
      buildLadder: function() {
         var self = this;

         var tip = self.showTip('stonehearth:build_ladder_title', 'stonehearth:build_ladder_description',
            {i18n: true});

         App.setGameMode('build');
         return this._callTool('buildLadder', function() {
            return radiant.call_obj(self._build_editor, 'build_ladder')
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
                  self.buildLadder();
               })
               .fail(function(response) {
                  self.hideTip(tip);
               });
         });
      },

      // item type is a uri, not an item entity
      removeLadder: function(ladder) {
         return radiant.call_obj(this._build_service, 'remove_ladder_command', ladder);
      },

      boxHarvestResources: function() {
         var self = this;

         var tip = self.showTip('stonehearth:harvest_resource_tip_title', 'stonehearth:harvest_resource_tip_description', {i18n : true});

         return this._callTool('boxHarvestResources', function() {
            return radiant.call('stonehearth:box_harvest_resources')
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:popup'} );
                  self.boxHarvestResources();
               })
               .fail(function(response) {
                  self.hideTip(tip);
               });
         });
      },

      createStockpile: function() {
         var self = this;

         App.setGameMode('zones');
         var tip = self.showTip('stonehearth:stockpile_tip_title', 'stonehearth:stockpile_tip_description', { i18n: true });

         return this._callTool('createStockpile', function() {
            return radiant.call('stonehearth:choose_stockpile_location')
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
                  radiant.call('stonehearth:select_entity', response.stockpile);
                  self.createStockpile();
               })
               .fail(function(response) {
                  self.hideTip(tip);
                  console.log('stockpile created!');
               });
         });
      },

      //TODO: make this available ONLY after a farmer has been created
      createFarm: function() {
         var self = this;

         App.setGameMode('zones');
         var tip = self.showTip('stonehearth:field_tip_title', 'stonehearth:field_tip_description', { i18n: true });

         return this._callTool('createFarm', function(){
            return radiant.call('stonehearth:choose_new_field_location')
            .done(function(response) {
               radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
               radiant.call('stonehearth:select_entity', response.field);
               self.createFarm();
            })
            .fail(function(response) {
               self.hideTip(tip);
               console.log('new field created!');
            });
         });
      },

      createTrappingGrounds: function() {
         var self = this;

         App.setGameMode('zones');
         var tip = self.showTip('stonehearth:trapper_zone_tip_title', 'stonehearth:trapper_zone_tip_description', { i18n: true });

         return this._callTool('createTrappingGrounds', function() {
            return radiant.call('stonehearth:choose_trapping_grounds_location')
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
                  radiant.call('stonehearth:select_entity', response.trapping_grounds);
                  self.createTrappingGrounds();
               })
               .fail(function(response) {
                  self.hideTip(tip);
               });
         });
      },

      digDown: function() {
         var self = this;

         App.setGameMode('mining');
         var tip = self.showTip('stonehearth:mine_down_tip_title', 'stonehearth:mine_down_tip_description', { i18n: true });

         return this._callTool('digDown', function() {
            return radiant.call('stonehearth:designate_mining_zone', 'down')
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
                  //radiant.call('stonehearth:select_entity', response.mining_zone);
                  self.digDown();
               })
               .fail(function(response) {
                  self.hideTip(tip);
               });
         });
      },

      digOut: function() {
         var self = this;

         App.setGameMode('mining');
         var tip = self.showTip('stonehearth:mine_out_tip_title', 'stonehearth:mine_out_tip_description', { i18n: true });

         return this._callTool('digOut', function() {
            return radiant.call('stonehearth:designate_mining_zone', 'out')
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
                  //radiant.call('stonehearth:select_entity', response.mining_zone);
                  self.digOut();
               })
               .fail(function(response) {
                  self.hideTip(tip);
               });
         });
      },

      undo: function () {
         radiant.call_obj(this._build_service, 'undo_command')
      },

      // Note: this fuction is COMPLETELY bogus.  The tool tip is wrong.  It's harcoded to load the
      // "blah" template, etc.
      drawTemplate: function(precall, template) {
         var self = this;

         var tip = self.showTip('stonehearth:draw_template_title', 'stonehearth:draw_template_description', { i18n: true });

         return this._callTool('drawTemplate', function() {
            return radiant.call_obj(self._build_editor, 'place_template', template)
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
                  self.drawTemplate(precall, template);
               })
               .fail(function(response) {
                  self.hideTip(tip);
               });
         }, precall);
      },

      buildWall: function(column, wall, precall) {
         var self = this;

         var tip = self.showTip('stonehearth:wall_segment_tip_title', 'stonehearth:wall_segment_tip_description', { i18n: true });

         return this._callTool('buildWall', function() {
            return radiant.call_obj(self._build_editor, 'place_new_wall', column, wall)
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
               })
               .fail(function(response) {
                  self.hideTip(tip);
               });
         }, precall);
      },

      buildFloor: function(floorBrush, precall) {
         var self = this;

         var tip = self.showTip('stonehearth:build_floor_tip_title', 'stonehearth:build_floor_tip_description', { i18n: true });

         return this._callTool('buildFloor', function() {
            return radiant.call_obj(self._build_editor, 'place_new_floor', floorBrush)
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
               })
               .fail(function(response) {
                  self.hideTip(tip);
               });
         }, precall);
      },

      buildSlab: function(slabBrush, precall) {
         var self = this;

         var tip = self.showTip('stonehearth:build_slab_tip_title', 'stonehearth:build_slab_tip_description', { i18n: true });

         return this._callTool('buildSlab', function() {
            return radiant.call_obj(self._build_editor, 'place_new_slab', slabBrush)
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
               })
               .fail(function(response) {
                  self.hideTip(tip);
               });
         }, precall);
      },

      eraseStructure: function(precall) {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:popup'} );
         var self = this;

         var tip = self.showTip('stonehearth:erase_structure_tip_title', 'stonehearth:erase_structure_tip_description', { i18n: true });

         return this._callTool('eraseStructure', function() {
            return radiant.call_obj(self._build_editor, 'erase_structure')
               .done(function(response) {                  
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
               })
               .fail(function(response) {
                  self.hideTip(tip);
               });
         }, precall);
      },

      buildRoad: function(roadBrush, precall) {
         var self = this;

         var tip = self.showTip('stonehearth:build_road_tip_title', 'stonehearth:build_road_tip_description', { i18n: true });

         return this._callTool('buildRoad', function() {
            return radiant.call_obj(self._build_editor, 'place_new_road', roadBrush)
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
               })
               .fail(function(response) {
                  self.hideTip(tip);
               });
         }, precall);
      },

      growRoof: function(roof, precall) {
         var self = this;

         var tip = self.showTip('stonehearth:roof_tip_title', 'stonehearth:roof_tip_description', { i18n: true });

         return this._callTool('growRoof', function() {
            return radiant.call_obj(self._build_editor, 'grow_roof', roof)
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
               });
         }, precall);
      },

      applyConstructionDataOptions: function(blueprint, options) {
         return radiant.call_obj(this._build_service, 'apply_options_command', blueprint.__self, options);
      },

      getCost: function(building) {
         return radiant.call_obj(this._build_service, 'get_cost_command', building);
      },

      setGrowRoofOptions: function(options) {
         return radiant.call_obj(this._build_editor, 'set_grow_roof_options', options);
      },

      growWalls: function(column, wall, precall) {
         var self = this;

         var tip = self.showTip('stonehearth:raise_walls_tip_title', 'stonehearth:raise_walls_tip_description', { i18n: true } );

         return this._callTool('growWalls', function() {
            return radiant.call_obj(self._build_editor, 'grow_walls', column, wall)
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'});
               });
         }, precall);
      },

      replaceStructure: function(old_structure, new_structure_uri) {
         var self = this;
         if (old_structure) {
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
            radiant.call_obj(self._build_service, 'substitute_blueprint_command', old_structure.__self, new_structure_uri)
               .done(function(o) {
                  if (o.new_selection) {
                     radiant.call('stonehearth:select_entity', o.new_selection);
                  }
               });
         }
      },

      addDoodad: function(doodadUri, precall) {
         var self = this;

         // xxx localize
         var tip = self.showTip('stonehearth:doodad_tip_title', 'stonehearth:doodad_tip_description', { i18n: true });

         return this._callTool('addDoodad', function() {
            return radiant.call_obj(self._build_editor, 'add_doodad', doodadUri)
               .done(function(response) {
                  radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
                  self.hideTip();
               });
         }, precall);
      },

      buildRoom: function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:popup'} );
         var self = this;
         return this._callTool('buildRoom', function() {
            return radiant.call_obj(self._build_editor, 'create_room');
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:place_structure'} );
         });
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


