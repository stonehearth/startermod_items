App.StonehearthBuildModeView = App.View.extend({
   templateName: 'buildMode',
   i18nNamespace: 'stonehearth',

   init: function() {
      this._super();

      var self = this;

      // track the selection
      $(top).on("radiant_selection_changed", function (_, e) {
         self._selectedEntity = e.selected_entity;
         self._onStateChanged()
      });

      // track game mode changes and nuke any UI that we've show when we exit build mode
      $(top).on('mode_changed', function(_, mode) {
         self._mode = mode;
         self._onStateChanged();
      });

      // show the building designer when the "design building" button on the start menu
      // is clicked.
      $(top).on('stonehearth_design_building', function() {
         self._showBuildingDesigner();
      });

      // show the palce item UI "place item" button on the start menu
      // is clicked.
      $(top).on('stonehearth_place_item', function() {
         self._showPlaceItemUi();
      });

   },
   
   didInsertElement: function() {
      this.$().hide();
   },

   // Nuke the trace on the selected entity
   destroy: function() {
      this._destroyBuildingDesigner();
      this._destroyPlaceItemUi();
      this._super();
   },

   _showBuildingDesigner: function(uri) {
      App.setGameMode('build');

      // hide all other views
      this._hideDesignerViews();

      // show the building designer
      if (this._buildDesignerTools) {
         this._buildDesignerTools.$().show();
      } else {      
         this._buildDesignerTools = App.gameView.addView(App.StonehearthBuildingDesignerTools);
      }

      this._buildDesignerTools.set('uri', uri);
   },

   _destroyBuildingDesigner: function() {
      if (this._buildDesignerTools) {
         this._buildDesignerTools.destroy();
         this._buildDesignerTools = null;
      }
   },

   _showPlaceItemUi: function(uri) {
      App.setGameMode('build');

      // hide all other views
      this._hideDesignerViews();

      if (this._placeItemUi) {
         this._placeItemUi.$().show();
      } else {      
         this._placeItemUi = App.gameView.addView(App.StonehearthPlaceItemView);
      }
   },

   _destroyPlaceItemUi: function() {
      if (this._placeItemUi) {
         this._placeItemUi.destroy();
         this._placeItemUi = null;
      }
   },

   _hideDesignerViews: function() {
      $('.buildAndDesignTool').parent().hide();
   },

   _destroyAllViews: function() {
      this._destroyBuildingDesigner();
      this._destroyPlaceItemUi();
   },

   _onStateChanged: function() {
      var self = this;

      if (this._mode == "build") {
      
         //trace the selected entity to determine its components   
         if (self.selectedEntityTrace) {
            self.selectedEntityTrace.destroy();
         }


         self.selectedEntityTrace = radiant.trace(this._selectedEntity)
            .progress(function(entity) {
               // if the selected entity is a building part, show the building designer
               if (entity['stonehearth:fabricator'] || entity['stonehearth:construction_data']) {
                  self._showBuildingDesigner(entity.__self);
               } else {
                  self._destroyBuildingDesigner();
               }
            })
            .fail(function(e) {
               console.log(e);
            });         
      } else {
         this._destroyAllViews();
      }

   },

});
