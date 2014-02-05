App.StonehearthEmbarkView = App.View.extend({
   templateName: 'stonehearthEmbark',
   i18nNamespace: 'stonehearth',

   init: function() {
      this._super();

      var self = this;
      var MAX_UINT32 = 4294967295;
      var seed = Math.floor(Math.random() * (MAX_UINT32+1));

      radiant.call('stonehearth:new_game', 5, 5, seed)
         .done(function(e) {
            $('#map').stonehearthMap({
               mapGrid: e.map,
               click: function(cellX, cellY) {
                  self._selectedX = cellX;
                  self._selectedY = cellY;
                  $("#embarkButton").show();
                  // what's the best practice for this?
                  $('#map').stonehearthMap('suspend');
               },
               hover: function(cellX, cellY) {
                  self._updateScroll(e.map[cellY][cellX]);
               },
            });
         })
         .fail(function(e) {
            console.error('new_game failed:', e)
         });
   },

   didInsertElement: function() {
      this._super();
      var self = this;

      $("#embarkButton").click(function() {
         self._embark(self._selectedX, self._selectedY);
      });

      $(document).on('keyup keydown', function(e) {
         // escape key
         if (e.keyCode == 27) {
            if ($('#map').stonehearthMap('suspended')) {
               $("#embarkButton").hide();
               self._updateScroll(null);
               // what's the best practice for this?
               $('#map').stonehearthMap('clearCrosshairs');
               $('#map').stonehearthMap('resume');
            }
         }
      });
   },

   _embark: function(cellX, cellY) {
      var self = this;

      radiant.call('stonehearth:generate_start_location', cellX, cellY);
      App.shellView.addView(App.StonehearthLoadingScreenView);
      self.destroy();
   },

   _updateScroll: function(cell) {
      var self = this;
      var terrainDescription = '';
      var vegetationDescription = '';
      var wildlifeDescription = '';

      if (cell != null) {
         terrainDescription = i18n.t(cell.terrain_code);
         vegetationDescription = i18n.t('vegetation_' + cell.vegetation_density);
         wildlifeDescription = i18n.t('wildlife_' + cell.wildlife_density);
      }

      self.my('#terrain_type').html(terrainDescription);
      self.my('#vegetation').html(vegetationDescription);
      self.my('#wildlife').html(wildlifeDescription);
   }
});
