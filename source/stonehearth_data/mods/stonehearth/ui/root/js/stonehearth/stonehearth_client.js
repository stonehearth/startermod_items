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

      getActiveTool: function() {
         return this._activeTool;
      },

      deactivateAllTools: function() {
         var self = this;
         return radiant.call('radiant:client:deactivate_all_tools')
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

      buildFloor: function() {
         var self = this;

         $(top).trigger('radiant_show_tip', { 
            title : 'Build Floor Tooltip',
            description : 'Build Floor Tooltip'
         });

         return this._callTool(function() {
            return radiant.call_obj(self._build_editor, 'place_new_floor')
               .always(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                  $(top).trigger('radiant_hide_tip');
               });
         });
      },

      growWalls: function() {
         var self = this;

         $(top).trigger('radiant_show_tip', { 
            title : 'Fill Wall Tooltip',
            description : 'Fill Wall Tooltip'
         });

         return this._callTool(function() {
            return radiant.call_obj(self._build_editor, 'grow_walls')
               .always(function(response) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                  $(top).trigger('radiant_hide_tip');
               });
         });
      },

      buildRoom: function() {
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
            this._citizenManager.$().hide();
         }

         if (this._crafterManager) {
            this._crafterManager.$().hide();
         }

         // toggle the town menu
         if (!this._townMenu) {
            this._townMenu = App.gameView.addView(App.StonehearthTownView);
         } else {
            this._townMenu.$().toggle();
         }         
      },

      _citizenManager: null,
      showCitizenManager: function(show) {
         // hide the other population managers....oh lord this is ugly code
         if (this._crafterManager) {
            this._crafterManager.$().hide();
         }

         if (this._townMenu) {
            this._townMenu.$().hide();  
         }

         // toggle the citizenManager
         if (!this._citizenManager) {
            this._citizenManager = App.gameView.addView(App.StonehearthCitizensView);
         } else {
            if (!this._citizenManager.$().is(":visible")) {
               this._citizenManager.preShow();
            }
            
            this._citizenManager.$().toggle();
         }
      },

      _crafterManager: null,
      showCrafterManager: function(show) {
         // hide the other population managers....oh lord this is ugly code
         if (this._citizenManager) {
            this._citizenManager.$().hide();
         }

         if (this._townMenu) {
            this._townMenu.$().hide();  
         }         
         
         // toggle the citizenManager
         if (!this._crafterManager) {
            this._crafterManager = App.gameView.addView(App.StonehearthCraftersView);
         } else {
            if (!this._crafterManager.$().is(":visible")) {
               this._crafterManager.preShow();
            }

            this._crafterManager.$().toggle();
         }
      },

   });
   App.stonehearthClient = new StonehearthClient();
})();


