App.StonehearthEmbarkView = App.View.extend({
   templateName: 'stonehearthEmbark',
   i18nNamespace: 'stonehearth',

   classNames: ['flex', 'fullScreen', 'embarkBackground'],

   // Game options (such as peaceful mode, etc.)
   options: {},

   init: function() {
      this._super();
      var self = this;
   },

   didInsertElement: function() {
      this._super();
      var self = this;

      self._newGame(function(e) {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:paper_menu'} );
         self.$('#map').stonehearthMap({
            mapGrid: e.map,

            click: function(cellX, cellY) {
               self._chooseLocation(cellX, cellY);
            },

            hover: function(cellX, cellY) {
               var map = $('#map').stonehearthMap('getMap');
               self._updateScroll(map[cellY][cellX]);
            }
         });
      });

      $('body').on( 'click', '#embarkButton', function() {
         self._embark(self._selectedX, self._selectedY);
      });

      $('body').on( 'click', '#clearSelectionButton', function() {
         self._clearSelection();
      });

      self.$("#regenerateButton").click(function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:reroll'} );
         self._clearSelection();
         self.$('#map').stonehearthMap('suspend');

         self._newGame(function(e) {
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:paper_menu'} );
            self.$('#map').stonehearthMap('setMap', e.map);
            self.$('#map').stonehearthMap('resume');
         });
      });

      $(document).on('keydown', this._clearSelectionKeyHandler);
   },

   destroy: function() {
      $(document).off('keydown', this._clearSelectionKeyHandler);
      this._super();
   },

   _chooseLocation: function(cellX, cellY) {
      var self = this;

      self._selectedX = cellX;
      self._selectedY = cellY;

      self.$('#map').stonehearthMap('suspend');

      // Must show before setting position. jQueryUI does not support positioning of hidden elements.
      self.$('#embarkPin').show();
      self.$('#embarkPin').position({
         my: 'left+' + 12 * cellX + ' top+' + 12 * cellY,
         at: 'left top',
         of: self.$('#map'),
      })

      var tipContent = '<div id="embarkTooltip">';
      tipContent += '<button id="embarkButton" class="flat">' + i18n.t('stonehearth:embark_at_this_location') + '</button><br>';
      tipContent += '<button id="clearSelectionButton" class="flat">' + i18n.t('stonehearth:embark_clear_selection') + '</button>';
      tipContent += '</div>'

      self.$('#embarkPin').tooltipster({
         autoClose: false,
         interactive: true,
         content:  $(tipContent)
      });

      self.$('#embarkPin').tooltipster('show');
   },

   _newGame: function(fn) {
      var self = this;
      var seed = self._generate_seed();

      radiant.call('stonehearth:new_game', 12, 8, seed, self.options)
         .done(function(e) {
            self._map_info = e.map_info
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
         self.$('#scroll').show();

         terrainType = i18n.t(cell.terrain_code);
         vegetationDescription = cell.vegetation_density;
         wildlifeDescription = cell.wildlife_density;
         mineralDescription = cell.mineral_density;

         if (cell.terrain_code != this._prevTerrainCode) {
            var portrait = 'url(/stonehearth/ui/shell/embark/images/' + cell.terrain_code + '.png)'

            self.$('#terrainType').html(terrainType);
            //self.$('#terrainPortrait').css('content', portrait);   
            
            this._prevTerrainCode = cell.terrain_code;
         }

         self._updateTileRatings(self.$('#vegetation'), cell.vegetation_density);
         self._updateTileRatings(self.$('#wildlife'), cell.wildlife_density);
         self._updateTileRatings(self.$('#minerals'), cell.mineral_density);
         
         /*
         self.$('#vegetation')
            .removeAttr('class')
            .addClass('level' + cell.vegetation_density)
            .html(vegetationDescription);

         self.$('#wildlife')
            .removeAttr('class')
            .addClass('level' + cell.wildlife_density)
            .html(wildlifeDescription);

         self.$('#minerals')
            .removeAttr('class')
            .addClass('level' + cell.mineral_density)
            .html(mineralDescription);
         */
      } else {
         self.$('#scroll').hide();
      }
   },

   _updateTileRatings: function(el, rating) {
      el.find('.bullet')
         .removeClass('full');

      for(var i = 1; i < rating + 1; i++) {
         el.find('.' + i).addClass('full');
      }
   },

   _clearSelection: function() {
      var self = this;

      try {
         self.$('#embarkPin').tooltipster('destroy');
         self.$('#embarkPin').hide();
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:carpenter_menu:menu_closed'} );
      } catch(e) {
      }

      self.$('#map').stonehearthMap('clearCrosshairs');
      self._updateScroll(null);

      if (self.$('#map').stonehearthMap('suspended')) {
         self.$('#map').stonehearthMap('resume');
      }
   },

   _clearSelectionKeyHandler: function(e) {
      var self = this;

      var escape_key_code = 27;

      if (e.keyCode == escape_key_code) {
         $('#clearSelectionButton').click();
      }
   },

   _embark: function(cellX, cellY) {
      var self = this;

      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:embark'} );
      radiant.call('stonehearth:generate_start_location', cellX, cellY, self._map_info);
      App.shellView.addView(App.StonehearthLoadingScreenView);
      self.destroy();
   }
});
