var GameModeManager;

/*
$(document).ready(function() {
   // when clicking on a tool, don't change modes
   $(document).on( 'click', '.tool', function() {

   }
});
*/

(function () {
   GameModeManager = SimpleClass.extend({

      modes: {
         NORMAL : "normal",
         ZONES : "zones",
         BUILD : "build",
         MINING : "mining",
      },

      views: {},

      _currentMode: null,
      _currentView: null,
      _currentVisionMode: 'normal',

      init: function() {
         var self = this;

         this._selectedEntityTraces = {};

         this.views[this.modes.ZONES] = App.StonehearthZonesModeView.create();
         this.views[this.modes.BUILD] = App.StonehearthBuildModeView.create();

         $(top).on("start_menu_activated", function(_, e) {
            self._onMenuActivated(e)
         });

         $(top).on("radiant_selection_changed.unit_frame", function (_, e) {
            self._onEntitySelected(e);
         });

         App.gameView.addView(this.views[this.modes.ZONES]);
         App.gameView.addView(this.views[this.modes.BUILD]);
      },

      setVisionMode: function(mode) {
         this._currentVisionMode = mode;
         radiant.call('stonehearth:set_building_vision_mode', this._currentVisionMode);
         $(document).trigger('stonehearthVisionModeChange', mode);
      },

      getVisionMode: function() {
         return this._currentVisionMode;
      },

      getGameMode: function() {
         return this._currentMode;
      },

      setGameMode: function (mode) {
         if (mode != this._currentMode) {
            //radiant.call('stonehearth:select_entity', null);
            App.stonehearthClient.deactivateAllTools();
            
            /*
            if (mode == this.modes.ZONES ||
                mode == this.modes.BUILD) {
               //pause
               radiant.call('stonehearth:dm_pause_game');
            } else {
               //unpause
               radiant.call('stonehearth:dm_resume_game');
            }
            */

            // hide the old mode view
            if (this._currentView) {
               this._currentView.hide();
            }

            // show the new mode view, if there is one
            var view = this.views[mode]
            if (view) {
               view.show();
            }
            
            this._currentView = view;
            this._currentMode = mode;
            
            // sync up the menu
            /*
            if (App.stonehearth.startMenu) {
               App.stonehearth.startMenu.stonehearthMenu('setGameMode', mode);
            }
            */

            // notify the rest of the ui
            $(top).trigger('mode_changed', mode);

            var hudMode = 'normal'

            if (mode == this.modes.ZONES ||
                mode == this.modes.BUILD ||
                mode == this.modes.MINING) {
               hudMode = 'hud';
            }

            if (hudMode != this._hudMode) {
               radiant.call('stonehearth:set_ui_mode', hudMode);
               this._hudMode = hudMode;
            }
         }
      },

      // if the selected menu is tagged with a game mode, switch to that mode
      _onMenuActivated: function(e) {
         if (!e.nodeData) {
            this.setGameMode(this.modes.NORMAL);
            return;
         }

         // if there's a mode associated with this menu, transition to that mode
         if (e.nodeData["game_mode"]) {
            this.setGameMode(e.nodeData["game_mode"]);
         }
      },

      // swich to the appropriate mode for the selected entity
      _onEntitySelected: function(e) {
         var self = this;
         var entity = e.selected_entity
         
         if (!entity) {
            //this.setGameMode(this.modes.NORMAL);
            return;
         }

         if (self.selectedEntityTrace) {
            self.selectedEntityTrace.destroy();
         }

         self.selectedEntityTrace = radiant.trace(entity)
            .progress(function(result) {
               self.setGameMode(self._getModeForEntity(result));
            })
            .fail(function(e) {
               console.log(e);
            });
      },

      _getModeForEntity: function(entity) {
         if ((!entity['stonehearth:ai'] && entity['stonehearth:storage']) ||
             entity['stonehearth:farmer_field'] ||
             entity['stonehearth:trapping_grounds'] ||
             entity['stonehearth:shepherd_pasture'] ||
             entity['stonehearth:mining_zone']) {
            return this.modes.ZONES;
         }

         if (entity['stonehearth:fabricator'] ||
             entity['stonehearth:construction_data'] ||
             entity['stonehearth:construction_progress']) {
            return this.modes.BUILD;
         }
         return this.modes.NORMAL;
      }
   });
})();
