App.StonehearthEmbarkView = App.View.extend({
   templateName: 'stonehearthEmbark',
   i18nNamespace: 'stonehearth',

   init: function() {
      this._super();
      var self = this;

      self._new_game(function(e) {
         $('#map').stonehearthMap({
            mapGrid: e.map,

            click: function(cellX, cellY) {
               self._selectedX = cellX;
               self._selectedY = cellY;

               $('#embarkButton').show();
               $('#clearSelectionButton').show();
               $('#map').stonehearthMap('suspend');
            },

            hover: function(cellX, cellY) {
               var map = $('#map').stonehearthMap('getMap');
               self._updateScroll(map[cellY][cellX]);
            }
         });
      });
   },

   didInsertElement: function() {
      this._super();
      var self = this;

      $("#embarkButton").click(function() {
         self._embark(self._selectedX, self._selectedY);
      });

      $("#clearSelectionButton").click(function() {
         self._clearSelection();
      });

      $("#regenerateButton").click(function() {
         self._clearSelection();

         self._new_game(function(e) {
            $('#map').stonehearthMap('setMap', e.map);
         });
      });

      $(document).on('keyup keydown', function(e) {
         var escape_key_code = 27;

         if (e.keyCode == escape_key_code) {
            self._clearSelection();
         }
      });
   },

   _new_game: function(fn) {
      var self = this;
      var seed = self._generate_seed();

      radiant.call('stonehearth:new_game', 14, 8, seed)
         .done(function(e) {
            fn(e);
         })
         .fail(function(e) {
            console.error('new_game failed:', e)
         });
   },

   _generate_seed: function() {
      // unsigned ints are marshalling across as signed ints to lua
      //var MAX_UINT32 = 4294967295;
      var MAX_INT32 = 2147483647;
      var seed = Math.floor(Math.random() * (MAX_INT32+1));
      return seed;
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
   },

   _clearSelection: function() {
      var self = this;

      $("#embarkButton").hide();
      $("#clearSelectionButton").hide();

      $('#map').stonehearthMap('clearCrosshairs');
      self._updateScroll(null);

      if ($('#map').stonehearthMap('suspended')) {
         $('#map').stonehearthMap('resume');
      }
   },

   _embark: function(cellX, cellY) {
      var self = this;

      radiant.call('stonehearth:generate_start_location', cellX, cellY);
      App.shellView.addView(App.StonehearthLoadingScreenView);
      self.destroy();
   }
});
