App.StonehearthEmbarkView = App.View.extend({
   templateName: 'stonehearthEmbark',
   i18nNamespace: 'stonehearth',

   classNames: ['flex', 'fullScreen', 'embarkBackground'],

   init: function() {
      this._super();
      var self = this;

      self._newGame(function(e) {
         $('#map').stonehearthMap({
            mapGrid: e.map,

            click: function(cellX, cellY) {
               self._chooseLocation(cellX, cellY)
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

      $('body').on( 'click', '#embarkButton', function() {
         self._embark(self._selectedX, self._selectedY);
      });

      $('body').on( 'click', '#clearSelectionButton', function() {
         self._clearSelection();
      });

      this.my("#regenerateButton").click(function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:reroll' );
         self._clearSelection();

         self._newGame(function(e) {
            self.my('#map').stonehearthMap('setMap', e.map);
         });
      });

      $(document).on('keyup keydown', function(e) {
         var escape_key_code = 27;

         if (e.keyCode == escape_key_code) {
            self._clearSelection();
         }
      });
   },

   _chooseLocation: function(cellX, cellY) {
      var self = this;

      self._selectedX = cellX;
      self._selectedY = cellY;

      $('#map').stonehearthMap('suspend');

      self.my('#embarkPin').position({
         my: 'left+' + 12 * cellX + ' top+' + 12 * cellY,
         at: 'left top',
         of: self.my('#map'),
      })

      var tipContent = '<div id="embarkTooltip">';
      tipContent += '<button id="embarkButton" class="flat">' + i18n.t('stonehearth:embark_at_this_location') + '</button><br>';
      tipContent += '<button id="clearSelectionButton" class="flat">' + i18n.t('stonehearth:embark_clear_selection') + '</button>';
      tipContent += '</div>'

      self.my('#embarkPin').tooltipster({
         autoClose: false,
         interactive: true,
         content:  $(tipContent)
      });

      self.my('#embarkPin').tooltipster('show');
   },

   _newGame: function(fn) {
      var self = this;
      var seed = self._generate_seed();

      radiant.call('stonehearth:new_game', 12, 8, seed)
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
      var terrainType = '';
      var vegetationDescription = '';
      var wildlifeDescription = '';

      if (cell != null) {
         $('#scroll').show();

         terrainType = i18n.t(cell.terrain_code);
         vegetationDescription = i18n.t('vegetation_' + cell.vegetation_density);
         wildlifeDescription = i18n.t('wildlife_' + cell.wildlife_density);

         if (cell.terrain_code != this._prevTerrainCode) {
            var portrait = 'url(/stonehearth/ui/shell/embark/images/' + cell.terrain_code + '.png)'

            self.my('#terrainType').html(terrainType);
            //self.my('#terrainPortrait').css('content', portrait);   
            
            this._prevTerrainCode = cell.terrain_code;
         }
         
         self.my('#vegetation')
            .removeAttr('class')
            .addClass('level' + cell.vegetation_density)
            .html(vegetationDescription);

         self.my('#wildlife')
            .removeAttr('class')
            .addClass('level' + cell.wildlife_density)
            .html(wildlifeDescription);

      } else {
         $('#scroll').hide();
      }
   },

   _clearSelection: function() {
      var self = this;

      try {
         self.my('#embarkPin').tooltipster('destroy');
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:carpenter_menu:menu_closed' );
      } catch(e) {
      }

      $('#map').stonehearthMap('clearCrosshairs');
      self._updateScroll(null);

      if ($('#map').stonehearthMap('suspended')) {
         $('#map').stonehearthMap('resume');
      }
   },

   _embark: function(cellX, cellY) {
      var self = this;

      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:embark' );
      radiant.call('stonehearth:generate_start_location', cellX, cellY);
      App.shellView.addView(App.StonehearthLoadingScreenView);
      self.destroy();
   }
});
