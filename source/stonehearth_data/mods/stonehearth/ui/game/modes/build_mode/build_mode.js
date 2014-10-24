App.StonehearthBuildModeView = App.ContainerView.extend({

   init: function() {
      this._super();

      var self = this;

      $(top).on("radiant_selection_changed", function (_, e) {
         self._onEntitySelected(e);
      });

      $(top).on('selected_sub_part_changed', function(_, sub_part) {
         self._selectedSubPart = sub_part;
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
      $(top).on('stonehearth_building_editor', function() {
         self._showBuildingDesignerView('editor');
      });

      // show the building editor
      $(top).on('stonehearth_building_overview', function() {
         self._showBuildingDesignerView('overview');
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
      //radiant.call('stonehearth:select_entity', null);
      App.setGameMode('build');
      this.hideAllViews();
      this._buildingTemplatesView.show();
   },

   _showPlaceItemUi: function() {
      App.setGameMode('build');
      this.hideAllViews();
      this._placeItemView.show();
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
   
   _onEntitySelected: function(e) {
      var self = this;
      var entity = e.selected_entity
      
      self.hideAllViews();

      // nuke the old trace
      if (self.selectedEntityTrace) {
         self.selectedEntityTrace.destroy();
      }

      if (entity) {
         // trace the properties so we can tell if we need to popup the properties window for the object
         self.selectedEntityTrace = radiant.trace(entity)
            .progress(function(result) {
               self._examineEntity(result);
            })
            .fail(function(e) {
               console.log(e);
            });
      }
   },

   _examineEntity: function(entity) {
      var self = this;
      
      if (!entity) {
         return;
      }

      if (entity['stonehearth:building']) {
         self._buildingDesignerView.show();
      } else if (entity['stonehearth:farmer_field']) {
         //this._showFarmUi(entity);
      } else if (entity['stonehearth:trapping_grounds']) {
         //this._showTrappingGroundsUi(entity);
      }
   },

   _onStateChanged: function() {
      var self = this;

      if (self._mode == "build") {
         //trace the selected entity to determine its components
         if (self.selectedSubPartTrace) {
            self.selectedSubPartTrace.destroy();
            self.selectedSubPartTrace = null;
         }

         if (self._selectedSubPart) {
            self.selectedSubPartTrace = radiant.trace(self._selectedSubPart)
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
