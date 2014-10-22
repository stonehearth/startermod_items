App.StonehearthBuildModeView = App.ContainerView.extend({

   init: function() {
      this._super();

      var self = this;

      $(top).on('selected_sub_part_changed', function(_, sub_part) {
         self._sub_selected_part = sub_part;
         self._onStateChanged();
      }); 

      // track game mode changes and nuke any UI that we've show when we exit build mode
      $(top).on('mode_changed', function(_, mode) {
         self._mode = mode;
         self._onStateChanged();
      });

      // show the building designer when the "design building" button on the start menu
      // is clicked.
      $(top).on('stonehearth_building_templates', function() {
         self._showBuildingTemplatesView();
         //self._showBuildingDesignerView();
      });

      // show the building editor
      $(top).on('stonehearth_building_designer', function() {
         self._showBuildingDesignerView();
      });

      // show the place item UI "place item" button on the start menu
      // is clicked.
      $(top).on('stonehearth_place_item', function() {
         self._showPlaceItemUi();
      });
   },
   
   didInsertElement: function() {
      var self = this;

      this._placeItemView = self.addView(App.StonehearthPlaceItemView);
      this._buildingTemplatesView = self.addView(App.StonehearthBuildingTemplatesView);
      this._buildingDesignerView = self.addView(App.StonehearthBuildingDesignerTools);

      this.hide();
   },

   // Nuke the trace on the selected entity
   destroy: function() {
      this._destroyBuildingDesigner();
      this._destroyPlaceItemUi();
      this._super();
   },

   _showBuildingTemplatesView: function() {
      radiant.call('stonehearth:select_entity', null);
      App.setGameMode('build');
      this.hideAllViews();
      this._buildingTemplatesView.show();
   },

   _showPlaceItemUi: function() {
      App.setGameMode('build');
      this.hideAllViews();
      this._placeItemView.show();
   },

   _showBuildingDesignerView: function() {
      App.setGameMode('build');
      this.hideAllViews();
      this._buildingDesignerView.show();
   },
   
   _onStateChanged: function() {
      var self = this;

      if (self._mode == "build") {
         //trace the selected entity to determine its components
         if (self.selectedEntityTrace) {
            self.selectedEntityTrace.destroy();
            self.selectedEntityTrace = null;
         }

         if (self._sub_selected_part) {
            self.selectedEntityTrace = radiant.trace(self._sub_selected_part)
               .progress(function(entity) {
                  // if the selected entity is a building part, show the building designer
                  if (entity['stonehearth:fabricator'] || entity['stonehearth:construction_data']) {
                     self._buildingDesignerView.set('uri', entity.__self);
                     self.hideAllViews();
                     self._buildingDesignerView.show();
                  } else {
                     self._buildingDesignerView.hide();
                  }
               })
               .fail(function(e) {
                  console.log(e);
               });         
         }
      } else {
         self.hideAllViews();
      }
   },
});
