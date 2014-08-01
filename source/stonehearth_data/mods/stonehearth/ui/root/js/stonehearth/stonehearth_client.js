var StonehearthClient;

(function () {
   StonehearthClient = SimpleClass.extend({

      init: function() {
         var self = this;

         self.rallyWorkersEnabled = false;

         radiant.call('stonehearth:get_client_service', 'build_editor')
            .done(function(e) {
               self._build_editor = e.result;
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
         return radiant.call('stonehearth:deactivate_all_tools')
            .always(function() {
               self._activeTool = null;
            });
      },

      // Wrapper to call all tools, handling the boilerplate tool management.
      _callTool: function(toolFunction) {
         var self = this;

         var deferred = new $.Deferred();

         this.deactivateAllTools()
            .always(function() {
               // when all tools are deactivated, activate the tool specified in the params
               self._activeTool = toolFunction()
                  .done(function(response) {
                     deferred.resolve(response);
                  })
                  .fail(function(response) {
                     deferred.reject(response);
                  })
                  .always(function (){
                     self._activeTool = null;
                  })
            });

         return deferred;
      },

      // item is a reference to an actual entity, not a class of entities like stonehearth:comfy_bed
      placeItem: function(item, o) {
         var self = this;

         if (!o || !o.hideTip) {
            $(top).trigger('radiant_show_tip', {
               title : i18n.t('stonehearth:item_placement_title'),
               description : i18n.t('stonehearth:item_placement_description')
            });
         }

         App.setGameMode('build');
         return this._callTool(function() {
            return radiant.call('stonehearth:choose_place_item_location', item)
               .done(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' )
               })
               .always(function(response) {
                  App.setGameMode('normal');
                  $(top).trigger('radiant_hide_tip');
               });
         });
      },

      // item type is a uri, not an item entity
      placeItemType: function(itemType, o) {
         var self = this;

         if (!o || !o.hideTip) {
            $(top).trigger('radiant_show_tip', {
               title : i18n.t('stonehearth:item_placement_title'),
               description : i18n.t('stonehearth:item_placement_description')
            });
         }

         App.setGameMode('build');
         return this._callTool(function() {
            return radiant.call('stonehearth:choose_place_item_location', itemType)
               .done(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' )
                  if (response.more_items) {
                     self.placeItemType(itemType, { hideTip : true });
                  }
               })
               .fail(function(response) {
                  App.setGameMode('normal');
                  $(top).trigger('radiant_hide_tip');
               });
         });
      },

      boxHarvestResources: function(o) {
         var self = this;

         if (!o || !o.hideTip) {
            $(top).trigger('radiant_show_tip', { 
               title : 'Click and drag to harvest resources',
               description : 'Drag out a box to choose which resources to harvest<p>Right click to exit.'
            });            
         }

         return this._callTool(function() {
            return radiant.call('stonehearth:box_harvest_resources')
               .done(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' );
                  self.boxHarvestResources({ hideTip : true });
               })
               .fail(function(response) {
                  $(top).trigger('radiant_hide_tip');
               });
         });
      },

      createStockpile: function(o) {
         var self = this;

         App.setGameMode('zones');

         // xxx, localize
         if (!o || !o.hideTip) {
            $(top).trigger('radiant_show_tip', { 
               title : 'Click and drag to create your stockpile',
               description : 'Your citizens place resources and crafted goods in stockpiles for safe keeping!'
            });
         }

         return this._callTool(function() {
            return radiant.call('stonehearth:choose_stockpile_location')
               .done(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                  radiant.call('stonehearth:select_entity', response.stockpile);
                  self.createStockpile({ hideTip : true });
               })
               .fail(function(response) {
                  $(top).trigger('radiant_hide_tip');
                  console.log('stockpile created!');
               });
         });
      },

      //TODO: make this available ONLY after a farmer has been created
      createFarm: function(o) {
         var self = this;

         App.setGameMode('zones');

         // xxx, localize
         if (!o || !o.hideTip) {
            $(top).trigger('radiant_show_tip', { 
               title : 'Click and drag to designate a new field.',
               description : 'Farmers will break ground and plant crops here'
            });
         }

         return this._callTool(function(){
            return radiant.call('stonehearth:choose_new_field_location')
            .done(function(response) {
               radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
               radiant.call('stonehearth:select_entity', response.field);
               self.createFarm({ hideTip : true });
            })
            .fail(function(response) {
               $(top).trigger('radiant_hide_tip');
               console.log('new field created!');
            });
         });
      },

      createTrappingGrounds: function(o) {
         var self = this;

         App.setGameMode('zones');

         //xxx localize
         if (!o || !o.hideTip) {
            $(top).trigger('radiant_show_tip', { 
               title : 'Click and drag to create trapping grounds.',
               description : 'Trappers catch critters for food and resources in this zone.'
            });
         }

         return this._callTool(function() {
            return radiant.call('stonehearth:choose_trapping_grounds_location')
               .done(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                  radiant.call('stonehearth:select_entity', response.trapping_grounds);
                  self.createTrappingGrounds({ hideTip : true });
               })
               .fail(function(response) {
                  $(top).trigger('radiant_hide_tip');
               });
         });
      },

      undo: function () {
         radiant.call_obj(this._build_service, 'undo_command')
      },

      buildWall: function(column, wall, o) {
         var self = this;

         $(top).trigger('radiant_show_tip', { 
            title : 'Click to place wall segments',
            description : 'Hold down SHIFT while clicking to draw connected walls!'
         });

         return this._callTool(function() {
            return radiant.call_obj(self._build_editor, 'place_new_wall', column, wall)
               .done(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                  self.buildWall(column, wall, { hideTip : true });
               })
               .fail(function(response) {
                  $(top).trigger('radiant_hide_tip');
               });               
         });
      },

      buildFloor: function(floorBrush, o) {
         var self = this;

         if (!o || !o.hideTip) {
            $(top).trigger('radiant_show_tip', { 
               title : 'Build Floor',
               description : 'Drag out rectangles to place bits of floor.<br><br>Right click to exit.'
            });
         }

         return this._callTool(function() {
            return radiant.call_obj(self._build_editor, 'place_new_floor', floorBrush)
               .done(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                  self.buildFloor(floorBrush, { hideTip : true });
               })
               .fail(function(response) {
                  $(top).trigger('radiant_hide_tip');
               });
         });
      },

      eraseFloor: function(o) {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' );
         var self = this;

         if (!o || !o.hideTip) {
            $(top).trigger('radiant_show_tip', { 
               title : 'Erase Floor Tooltip',
               description : 'Erase Floor Tooltip'
            });
         }

         return this._callTool(function() {
            return radiant.call_obj(self._build_editor, 'erase_floor')
               .done(function(response) {                  
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                  self.eraseFloor({ hideTip : true });
               })
               .fail(function(response) {
                  $(top).trigger('radiant_hide_tip');
               });
         });
      },

      growRoof: function(roof) {
         var self = this;

         return this._callTool(function() {
            radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
            return radiant.call_obj(self._build_editor, 'grow_roof', roof);
         });
      },

      applyConstructionDataOptions: function(blueprint, options) {
         return radiant.call_obj(this._build_service, 'apply_options_command', blueprint.__self, options);
      },

      setGrowRoofOptions: function(options) {
         return radiant.call_obj(this._build_editor, 'set_grow_roof_options', options);
      },

      growWalls: function(column, wall) {
         var self = this;
         return this._callTool(function() {
            radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
            return radiant.call_obj(self._build_editor, 'grow_walls', column, wall)
         });
      },

      replaceStructure: function(old_structure, new_structure_uri) {
         var self = this;
         if (old_structure) {
            radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
            radiant.call_obj(self._build_service, 'substitute_blueprint_command', old_structure.__self, new_structure_uri)
               .done(function(o) {
                  if (o.new_selection) {
                     radiant.call('stonehearth:select_entity', o.new_selection);
                  }
               });
         }
      },

      addDoodad: function(doodadUri, o) {
         var self = this;

         $(top).trigger('radiant_show_tip', { 
            title : 'Place Object',
            description : 'Click to place.  ESC to cancel.'
         });

         return this._callTool(function() {
            return radiant.call_obj(self._build_editor, 'add_doodad', doodadUri)
               .always(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                  $(top).trigger('radiant_hide_tip');
               });
         });
      },

      buildRoom: function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' );
         var self = this;
         return this._callTool(function() {
            return radiant.call_obj(self._build_editor, 'create_room');
            radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
         });
      },

      _townMenu: null,
      showTownMenu: function(show) {
         // hide the other population managers....oh lord this is ugly code
         if (this._citizenManager) {
            this._citizenManager.destroy();
         }

         if (this._crafterManager) {
            this._crafterManager.destroy();
         }

         // toggle the town menu
         if (!this._townMenu || this._townMenu.isDestroyed) {
            this._townMenu = App.gameView.addView(App.StonehearthTownView);
         } else {
            this._townMenu.destroy();
         }         
      },

      _citizenManager: null,
      showCitizenManager: function(show) {
         // hide the other population managers....oh lord this is ugly code
         if (this._crafterManager) {
            this._crafterManager.destroy();
         }

         if (this._townMenu) {
            this._townMenu.destroy();
         }

         // toggle the citizenManager
         if (!this._citizenManager || this._citizenManager.isDestroyed) {
            this._citizenManager = App.gameView.addView(App.StonehearthCitizensView);
         } else {
            this._citizenManager.destroy();
         }
      },

      _crafterManager: null,
      showCrafterManager: function(show) {
         // hide the other population managers....oh lord this is ugly code
         if (this._citizenManager) {
            this._citizenManager.destroy();
         }

         if (this._townMenu) {
            this._townMenu.destroy();
         }         
         
         // toggle the citizenManager
         if (!this._crafterManager || this._crafterManager.isDestroyed) {
            this._crafterManager = App.gameView.addView(App.StonehearthCraftersView);
         } else {
            this._crafterManager.destroy();
         }
      },

      _redAlertWidget: null,
      rallyWorkers: function() {
         // TODO: add sound and visual indicator that this state is on
         if (self.rallyWorkersEnabled) {
            radiant.call('stonehearth:disable_worker_combat');
            radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:scenarios:redalert_off' );
            self.rallyWorkersEnabled = false;
            if (self._redAlertWidget) {
               self._redAlertWidget.destroy();   
            }
         } else {
            radiant.call('stonehearth:enable_worker_combat');
            self.rallyWorkersEnabled = true;
            self._redAlertWidget = App.gameView.addView(App.StonehearthRedAlertWidget);
         }
      },

   });
   App.stonehearthClient = new StonehearthClient();
})();


