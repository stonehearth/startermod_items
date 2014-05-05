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
         BUILD : "build"
      },

      views: {},

      _currentMode: null,
      _currentView: null,

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

      getGameMode: function() {
         return this._currentMode;
      },

      setGameMode: function (mode) {
         if (mode != this._currentMode) {

            if (mode == this.modes.ZONES ||
                mode == this.modes.BUILD) {
               //pause
               radiant.call('stonehearth:dm_pause_game');
            } else {
               //unpause
               radiant.call('stonehearth:dm_resume_game');
            }

            // hide the old mode view
            if (this._currentView) {
               this._currentView.$().fadeOut();
            }

            // show the new mode view, if there is one
            var view = this.views[mode]
            if (view) {
               view.$().fadeIn();
            }
            
            this._currentView = view;
            this._currentMode = mode;
            
            // sync up the menu
            if (App.stonehearth.startMenu) {
               App.stonehearth.startMenu.stonehearthMenu('setGameMode', mode);
            }

            // notify the rest of the ui
            $(top).trigger('mode_changed', mode);

            var hudMode = 'normal'

            if (mode == this.modes.ZONES ||
                mode == this.modes.BUILD) {
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
            this.setGameMode(this.modes.NORMAL);
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
         if (entity['stonehearth:stockpile'] ||
             entity['stonehearth:farmer_field']) {
            return this.modes.ZONES;
         }

         if (entity['stonehearth:fabricator'] ||
             entity['stonehearth:construction_data']) {
            if (this._currentMode == this.modes.ZONES ||
                this._currentMode == this.modes.BUILD ) {
               return this.modes.BUILD;
            }
         }
         return this.modes.NORMAL;
      }
   });
})();
