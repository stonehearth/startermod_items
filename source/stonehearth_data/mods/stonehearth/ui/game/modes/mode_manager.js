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

         App.gameView.insertAt(0, this.views[this.modes.ZONES]);
         App.gameView.insertAt(0, this.views[this.modes.BUILD]);
      },

      getGameMode: function() {
         return this._currentMode;
      },

      setGameMode: function (mode) {
         if (mode != this._currentMode) {

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
            App.stonehearth.startMenu.stonehearthMenu("setGameMode", mode);
           
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
         var mode = this.modes.NORMAL;

         if (entity['stonehearth:stockpile']) {
            mode = this.modes.ZONES;
         }

         return mode;
      }
   });
})();
