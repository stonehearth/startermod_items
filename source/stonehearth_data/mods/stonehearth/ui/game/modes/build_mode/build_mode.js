App.StonehearthBuildModeView = App.View.extend({
   templateName: 'buildMode',
   i18nNamespace: 'stonehearth',
   classNames: ['fullScreen', 'flex', 'modeBackground'],

   init: function() {
      this._super();

      var self = this;

      // track the selection
      $(top).on("radiant_selection_changed", function (_, e) {
         self._onEntitySelected(e);
      });

      /*
      $(top).on('select_stockpile', function(_, stockpile) {
         self._showStockpileUi(stockpile);
      });
      */

      // track game mode changes and nuke any UI that we've show when we exit build mode
      $(top).on('mode_changed', function(_, mode) {
         if (mode != 'build') {
            self._destroyCurrentSubView();
         } 
      });

      // show the building designer when the "design building" button on the start menu
      // is clicked.
      $(top).on('stonehearth_design_building', function() {
         self._showBuildingDesigner();
      });

   },
   
   didInsertElement: function() {
      this.$().hide();
   },

   // Nuke the trace on the selected entity
   destroy: function() {
      if (this.selectedEntityTrace) {
         this.selectedEntityTrace.destroy();
         this.selectedEntityTrace = null;
      }

      this._destroyCurrentSubView();
      this._super();
   },

   _showBuildingDesigner: function() {
      this._destroyCurrentSubView();
      this._subView = App.gameView.addView(App.StonehearthBuildingDesignerView);
   },

   _destroyCurrentSubView: function() {
      if (this._subView) {
         this._subView.destroy();
      }
   },

   // when an entity is selected, trace it to determine if the UI cares about that
   // entity and how to respond
   _onEntitySelected: function(e) {
      var self = this;
      var entity = e.selected_entity
      
      if (!entity) {
         return;
      }

      // nuke the old trace
      if (self.selectedEntityTrace) {
         self.selectedEntityTrace.destroy();
      }

      // trace the properties so we can tell if we need to popup the properties window for the object
      self.selectedEntityTrace = radiant.trace(entity)
         .progress(function(result) {
            self._examineEntity(result);
         })
         .fail(function(e) {
            console.log(e);
         });
   },

   // make the UI respond to the entity that has been selected. Show/hide hits of UI, etc.
   _examineEntity: function(entity) {

      // this is the implementation in the zones mode. The build mode will do something similar I imagine
      /*
      if (!entity && this._subView) {
         this._destroyCurrentSubView();
      }

      if (entity['stonehearth:stockpile']) {
         this._showStockpileUi(entity);
      } else if (entity['stonehearth:farmer_field']) {
         this._showFarmUi(entity);
      }
      */
   },

   _showStockpileUi: function(entity) {
      // not applicable for the build mode. Shown as an example
      /* 
      var self = this;

      this._destroyCurrentSubView();

      var uri = typeof(entity) == 'string' ? entity : entity.__self;
      
      this._subView = App.gameView.addView(App.StonehearthStockpileView, { 
            uri: uri,
            position: {
               my : 'center bottom',
               at : 'left+' + App.stonehearthClient.mouseX + " " + 'top+' + (App.stonehearthClient.mouseY - 10),
               of : $(document),
               collision : 'fit'
            }
         });
      */
   },

});
