var StonehearthClient;

(function () {
   StonehearthClient = SimpleClass.extend({

      init: function() {
         var self = this;

         radiant.call('stonehearth:get_build_editor')
            .done(function(e) {
               self._build_editor = e.build_editor;
            })
            .fail(function(e) {
               console.log('error getting build editor')
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

      boxHarvestResources: function() {
         $(top).trigger('radiant_show_tip', { 
            title : 'Click and drag to harvest resources',
            description : 'Drag out a box to choose which resources to harvest'
         });

         return this._callTool(function() {
            return radiant.call('stonehearth:box_harvest_resources')
               .always(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' );
                  $(top).trigger('radiant_hide_tip');
               });
         });
      },

      createStockpile: function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' );
         
         // xxx, localize
         $(top).trigger('radiant_show_tip', { 
            title : 'Click and drag to create your stockpile',
            description : 'Your citizens place resources and crafted goods in stockpiles for safe keeping!'
         });

         return this._callTool(function() {
            return radiant.call('stonehearth:choose_stockpile_location')
               .done(function(response) {
                  radiant.call('radiant:client:select_entity', response.stockpile);
               })
               .always(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                  $(top).trigger('radiant_hide_tip');
                  console.log('stockpile created!');
               });
         });
      },

      //TODO: make this available ONLY after a farmer has been created
      createFarm: function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' );
      
         // xxx, localize
         $(top).trigger('radiant_show_tip', { 
            title : 'Click and drag to designate a new field.',
            description : 'Farmers will break ground and plant crops here'
         });

         return this._callTool(function(){
            return radiant.call('stonehearth:choose_new_field_location')
            .done(function(response) {
               radiant.call('radiant:client:select_entity', response.field);
            })
            .always(function(response) {
               radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
               $(top).trigger('radiant_hide_tip');
               console.log('new field created!');
            });
         });
      },

      buildWall: function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' );
         var self = this;

         $(top).trigger('radiant_show_tip', { 
            title : 'Click to place wall segments',
            description : 'Hold down SHIFT while clicking to draw connected walls!'
         });

         return this._callTool(function() {
            return radiant.call_obj(self._build_editor, 'place_new_wall')
               .always(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                  $(top).trigger('radiant_hide_tip');
               });
         });
      },

      buildFloor: function(floor) {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' );
         var self = this;

         $(top).trigger('radiant_show_tip', { 
            title : 'Build Floor Tooltip',
            description : 'Build Floor Tooltip'
         });

         return this._callTool(function() {
            return radiant.call_obj(self._build_editor, 'place_new_floor', floor.brush)
               .always(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                  $(top).trigger('radiant_hide_tip');
               });
         });
      },

      eraseFloor: function(floor) {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' );
         var self = this;

         $(top).trigger('radiant_show_tip', { 
            title : 'Erase Floor Tooltip',
            description : 'Erase Floor Tooltip'
         });

         return this._callTool(function() {
            return radiant.call_obj(self._build_editor, 'erase_floor')
               .always(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                  $(top).trigger('radiant_hide_tip');
               });
         });
      },

      growRoof: function(building) {
         var self = this;
         if (building && building.__self) {
            radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
            return radiant.call_obj(self._build_editor, 'grow_roof', building.__self)
         }
      },

      growWalls: function(building, column, wall) {
         var self = this;
         if (building && building.__self) {
            radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
            return radiant.call_obj(self._build_editor, 'grow_walls', building.__self, column, wall)
         }
      },

      addDoodad: function(doodad) {
         var self = this;

         $(top).trigger('radiant_show_tip', { 
            title : 'Add ' + doodad.name,
            description : 'Click to place.  ESC to cancel.'
         });

         return this._callTool(function() {
            return radiant.call_obj(self._build_editor, 'add_doodad', doodad.uri)
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

   });
   App.stonehearthClient = new StonehearthClient();
})();


