App.StonehearthZonesModeView = App.View.extend({
   templateName: 'zonesMode',
   i18nNamespace: 'stonehearth',

   _subViews: {},

   init: function() {
      this._super();

      var self = this;
      $(top).on("radiant_selection_changed", function (_, e) {
         self._onEntitySelected(e);
      });

      $(top).on('mode_changed', function(_, mode) {
         if (mode != 'zones') {
            self._hideZoneViews();
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
      this._hideZoneViews();

      if (entity['stonehearth:stockpile']) {
         this._showStockpileUi(entity);
      } else if (entity['stonehearth:farmer_field']) {
         this._showFarmUi(entity);
      } else if (entity['stonehearth:trapping_grounds']) {
         this._showTrappingGroundsUi(entity);
      }
   },

   _hideZoneViews: function() {
      $.each(this._subViews, function(i, view) {
         if (view && !view.isDestroyed) {
            view.hide();
            //this._stockpileView.invokeDestroy();
         }
      })
   },

   _showStockpileUi: function(entity) {
      var self = this;

      var uri = typeof(entity) == 'string' ? entity : entity.__self;
      
      if (!self._stockpileView || self._stockpileView.isDestroyed) {
         self._stockpileView = App.gameView.addView(App.StonehearthStockpileView, { 
               uri: uri,
               position_hide: {
                  my : 'center bottom',
                  at : 'left+' + App.stonehearthClient.mouseX + " " + 'top+' + (App.stonehearthClient.mouseY - 10),
                  of : $(document),
                  collision : 'fit'
               }
            }); 

         self._subViews[App.StonehearthStockpileView] = this._stockpileView;
      } else {
         self._stockpileView.set('uri', uri);
         self._stockpileView.show();
      }
   },

   _showFarmUi: function(entity) {
      var self = this;

      var uri = typeof(entity) == 'string' ? entity : entity.__self;

      if (!self._farmView || self._farmView.isDestroyed) {
         self._farmView = App.gameView.addView(App.StonehearthFarmView, { 
               uri: uri,
               position_hide: {
                  my : 'center bottom',
                  at : 'left+' + App.stonehearthClient.mouseX + " " + 'top+' + (App.stonehearthClient.mouseY - 10),
                  of : $(document),
                  collision : 'fit'
               }
         });
         self._subViews[App.StonehearthFarmView] = self._farmView;
      } else {
         self._farmView.set('uri', uri);
         self._farmView.show();
      }
   },

   _showTrappingGroundsUi: function(entity) {
      var self = this;

      var uri = typeof(entity) == 'string' ? entity : entity.__self;
      
      // TODO: refactor parameters with stockpile and farm?
      if (!self._trappingGroundsView || self._trappingGroundsView.isDestroyed) {
         self._trappingGroundsView = App.gameView.addView(App.StonehearthTrappingGroundsView, { 
               uri: uri,
               position_hide: {
                  my : 'center bottom',
                  at : 'left+' + App.stonehearthClient.mouseX + " " + 'top+' + (App.stonehearthClient.mouseY - 10),
                  of : $(document),
                  collision : 'fit'
               }
            });
         self._subViews[App.StonehearthTrappingGroundsView] = self._trappingGroundsView;
      } else {
         self._trappingGroundsView.set('uri', uri);
         self._trappingGroundsView.show();         
      }
   }
});
