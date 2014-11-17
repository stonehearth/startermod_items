App.StonehearthBuildModeView = App.ContainerView.extend({

   fabProperties: {
     'stonehearth:fabricator' : {
         'blueprint': {
            'stonehearth:floor' : {}
         }
      }      
   },

   init: function() {
      this._super();

      var self = this;

      $(top).on('selected_sub_part_changed', function(_, change) {
         self._selectedSubPart = change.selected_sub_part;
         self._onStateChanged();
      });

      // track game mode changes and nuke any UI that we've show when we exit build mode
      $(top).on('mode_changed', function(_, mode) {
         self._onStateChanged();
      });

      // show the building designer when the "design building" button on the start menu
      // is clicked.
      $(top).on('stonehearth_building_templates', function() {
         self._showBuildingTemplatesView();
         //self._showBuildingDesignerView();
      });

      // show the building editor
      $(top).on('stonehearth_building_editor', function() {
         self._showBuildingDesignerView('editor');
      });

      // show the building editor
      $(top).on('stonehearth_building_overview', function() {
         self._showBuildingDesignerView('overview');
      });

      // show the road ui
      $(top).on('stonehearth_build_road', function() {
         self._showBuildRoadsView();
      });

      // show the mining ui
      $(top).on('stonehearth_mining', function() {
         self._showMiningView();
      });

      // show the place item UI "place item" button on the start menu
      // is clicked.
      $(top).on('stonehearth_place_item', function() {
         self._showPlaceItemView();
      });
   },
   
   didInsertElement: function() {
      var self = this;

      this._placeItemView = self.addView(App.StonehearthPlaceItemView);
      this._buildingTemplatesView = self.addView(App.StonehearthBuildingTemplatesView);
      this._buildingDesignerView = self.addView(App.StonehearthBuildingDesignerTools);
      this._buildRoadsView = self.addView(App.StonehearthBuildRoadsView);
      this._miningView = self.addView(App.StonehearthMiningView);

      this.hideAllViews();
      this.hide();
   },

   // Nuke the trace on the selected entity
   destroy: function() {
      this._destroyBuildingDesigner();
      this._destroyPlaceItemUi();
      this._super();
   },

   _showBuildingTemplatesView: function() {
      //radiant.call('stonehearth:select_entity', null);
      App.setGameMode('build');
      this.hideAllViews();
      this._buildingTemplatesView.show();
   },

   _showPlaceItemView: function() {
      App.setGameMode('build');
      this.hideAllViews();
      this._placeItemView.show();
   },

   _showBuildRoadsView: function() {
      App.setGameMode('build');
      this.hideAllViews();
      this._buildRoadsView.show();
   },

   _showMiningView: function() {
      App.setGameMode('build');
      this.hideAllViews();
      this._miningView.show();
   },

   _showBuildingDesignerView: function(mode) {
      App.setGameMode('build');
      this.hideAllViews();

      if (mode == 'editor') {
         this._buildingDesignerView.showEditor()
      } else {
         this._buildingDesignerView.showOverview()
      }

      this._buildingDesignerView.show();
   },

   _isRoad: function(entity) {
      return entity['stonehearth:fabricator'] && 
            entity['stonehearth:fabricator'].blueprint['stonehearth:floor'] &&
            entity['stonehearth:fabricator'].blueprint['stonehearth:floor'].category == 2;
   },

   _onStateChanged: function() {
      var self = this;

      if (App.getGameMode() == 'build') {
         //trace the selected entity to determine its components
         if (self.selectedSubPartTrace) {
            self.selectedSubPartTrace.destroy();
            self.selectedSubPartTrace = null;
         }

         if (self._selectedSubPart) {
            self.selectedSubPartTrace = new RadiantTrace();
            self.selectedSubPartTrace.traceUri(self._selectedSubPart, self.fabProperties)
               .progress(function(entity) {
                  // if the selected entity is a building part, show the building designer
                  if (entity['stonehearth:fabricator'] || entity['stonehearth:construction_data']) {
                     if (self._isRoad(entity)) {
                        // Unless it's a road!  Then, show the road UI.
                        self._buildRoadsView.set('uri', entity.__self);
                        self.hideAllViews();
                        self._buildRoadsView.show();
                     } else {
                        self.hideAllViews();
                        self._buildingDesignerView.set('uri', entity.__self);
                        self._buildingDesignerView.show();
                     }
                  } else {
                     self._buildingDesignerView.hide();
                  }
               })
               .fail(function(e) {
                  console.log(e);
               });         
         } else {
            self._buildingDesignerView.set('uri', null);
            self._buildRoadsView.set('uri', null);
         }
      } else {
         self.hideAllViews();
      }
   },
});
