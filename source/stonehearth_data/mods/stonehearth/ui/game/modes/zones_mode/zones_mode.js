App.StonehearthZonesModeView = App.View.extend({
   templateName: 'zonesMode',
   i18nNamespace: 'stonehearth',

   init: function() {
      this._super();

      var self = this;
      $(top).on("radiant_selection_changed", function (_, e) {
         self._onEntitySelected(e);
      });

      $(top).on('mode_changed', function(_, mode) {
         if (mode != 'zones') {
            if (self._propertyView) {
               self._propertyView.destroy();
            }
         } 
      });
   },
   
   didInsertElement: function() {
      this.$().hide();
   },

   destroy: function() {
      if (this.selectedEntityTrace) {
         this.selectedEntityTrace.destroy();
         this.selectedEntityTrace = null;
      }

      this._super();
   },

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

   _examineEntity: function(entity) {
      if (!entity && this._propertyView) {
         this._propertyView.destroy();
      }

      if (entity['stonehearth:storage'] && !entity['stonehearth:ai']) {
         // TODO: sigh, the above is probably wrong, but highly convenient.
         this._showStockpileUi(entity);
      } else if (entity['stonehearth:farmer_field']) {
         this._showFarmUi(entity);
      } else if (entity['stonehearth:trapping_grounds']) {
         this._showTrappingGroundsUi(entity);
      } else if (entity['stonehearth:mining_zone']) {
         this._showMiningZoneUi(entity);
      } else if (entity['stonehearth:shepherd_pasture']) {
         this._showPastureUi(entity);
      }
   },

   _showStockpileUi: function(entity) {
      var self = this;

      if (this._propertyView) {
         this._propertyView.destroy();
      };

      var uri = typeof(entity) == 'string' ? entity : entity.__self;
      
      this._propertyView = App.gameView.addView(App.StonehearthStockpileView, { uri: uri });
   },

   _showFarmUi: function(entity) {
      var self = this;

      if (this._propertyView) {
         this._propertyView.destroy();
      };

      var uri = typeof(entity) == 'string' ? entity : entity.__self;

      this._propertyView = App.gameView.addView(App.StonehearthFarmView, { uri: uri });
   },

   _showTrappingGroundsUi: function(entity) {
      var self = this;

      if (this._propertyView) {
         this._propertyView.destroy();
      };

      var uri = typeof(entity) == 'string' ? entity : entity.__self;

      this._propertyView = App.gameView.addView(App.StonehearthTrappingGroundsView, { uri: uri });
   },

   _showMiningZoneUi: function(entity) {
      var self = this;

      if (this._propertyView) {
         this._propertyView.destroy();
      };

      var uri = typeof(entity) == 'string' ? entity : entity.__self;
      
      this._propertyView = App.gameView.addView(App.StonehearthMiningZoneView, { uri: uri });
   },

   _showPastureUi: function(entity) {
      var self = this;

      if (this._propertyView) {
         this._propertyView.destroy();
      };

      var uri = typeof(entity) == 'string' ? entity : entity.__self;
      
      this._propertyView = App.gameView.addView(App.StonehearthPastureView, { uri: uri });
   }

});
