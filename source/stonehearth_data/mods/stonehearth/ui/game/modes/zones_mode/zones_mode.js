App.StonehearthZonesModeView = App.View.extend({
   templateName: 'zonesMode',
   i18nNamespace: 'stonehearth',
   classNames: ['fullScreen', 'flex', 'modeBackground'],

   init: function() {
      this._super();

      var self = this;
      $(top).on("radiant_selection_changed", function (_, e) {
         self._onEntitySelected(e);
      });

      $(top).on('select_stockpile', function(_, stockpile) {
         self._showStockpileUi(stockpile);
      });

      $(top).on('select_farm', function(_, farm) {
         self._showFarmUi(farm);
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

      if (entity['stonehearth:stockpile']) {
         this._showStockpileUi(entity);
      } else if (entity['stonehearth:farmer_field']) {
         this._showFarmUi(entity);
      }
   },

   _showStockpileUi: function(entity) {
      var self = this;

      if (this._propertyView) {
         this._propertyView.destroy();
      };

      var uri = typeof(entity) == 'string' ? entity : entity.__self;
      
      this._propertyView = App.gameView.addView(App.StonehearthStockpileView, { 
            uri: uri,
            position: {
               my : 'center bottom',
               at : 'left+' + App.stonehearthClient.mouseX + " " + 'top+' + (App.stonehearthClient.mouseY - 10),
               of : $(document),
               collision : 'fit'
            }
         });
   },

   _showFarmUi: function(entity) {
      var self = this;

      if (this._propertyView) {
         this._propertyView.destroy();
      };

      var uri = typeof(entity) == 'string' ? entity : entity.__self;

      this._propertyView = App.gameView.addView(App.StonehearthFarmView, { 
            uri: uri,
            position: {
               my : 'center bottom',
               at : 'left+' + App.stonehearthClient.mouseX + " " + 'top+' + (App.stonehearthClient.mouseY - 10),
               of : $(document),
               collision : 'fit'
            }
      });

   }
});
