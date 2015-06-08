App.StonehearthEmbarkView = App.View.extend({
   templateName: 'stonehearthEmbark',
   i18nNamespace: 'stonehearth',

   // Game options (such as peaceful mode, etc.)
   _options: {},
   _components: {
      "citizens" : {
         "*" : {
            "unit_info": {},
            "stonehearth:attributes": {
               "attributes" : {}
            },
         }
      }
   },

   init: function() {
      this._super();
      var self = this;
      this._trace = new StonehearthDataTrace(App.population.getUri(), this._components);

      this._shopItemData = null;
      this._startingGold = 0;
      this._shopPalette;

      this._citizensArray = [];
   },

   didInsertElement: function() {
      this._super();
      var self = this;
      $('body').on( 'click', '#embarkButton', function() {
         self._embark();
      });

      self.$("#regenerateButton").click(function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:reroll'} );

         radiant.call_obj('stonehearth.game_creation', 'generate_citizens_command')
            .done(function(e) {
            })
            .fail(function(e) {
               console.error('generate_citizens failed:', e)
            });
      });

      radiant.call_obj('stonehearth.game_creation', 'generate_citizens_command')
         .done(function(e) {
            self._trace.progress(function(pop) {
               self.set('model', pop);
               self._buildCitizensArray();
            });
         })
         .fail(function(e) {
            console.error('generate_citizens failed:', e)
         });

      // Citizen Roster
      this._citizensRoster = this.$('#citizensRoster').stonehearthRoster();

      // Shop
      self.$('#resetButton').click(function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:popup'} )
         self._doReset();
      });

      this._shopPalette = this.$('#shopItems').stonehearthEmbarkShopPalette(
         {
            canSelect: function(item) {
               var cost = parseInt(item.attr('cost'));
               var gold = self.get('gold');
               return cost <= gold;
            },

            click: function(item) {
               self._doBuy(item);
            }
         }
      );

      $.getJSON('/stonehearth/ui/data/embark_shop.json', function(data) {
         
         self._startingGold = data.starting_gold;
         self.set('gold', self._startingGold);

         self._shopItemData = []
         var modules = App.getModuleData();
         radiant.each(data.shop_items, function(i, itemData) {
            var isPet = itemData.is_pet ? true : false;
            if (!isPet || modules.kickstarter_pets) {
               // Skip if item is a pet and kickstarter pets not enabled.
               var item = {
                  "displayName" : itemData.display_name,
                  "description": itemData.description,
                  "uri" : itemData.uri,
                  "isPet" : isPet,
                  "icon" : itemData.icon,
                  "cost" : itemData.cost
               };
               self._shopItemData.push(item);
            }
            
         })

         self._shopPalette.stonehearthEmbarkShopPalette('updateItems', self._shopItemData);
      });

      $(document).on('keydown', this._clearSelectionKeyHandler);
   },

   destroy: function() {
      if (this._trace) {
         this._trace.destroy();
      }
      $(document).off('keydown', this._clearSelectionKeyHandler);
      this._super();
   },

   _embark: function() {
      var self = this;
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:embark'});

      // Find the bought items and push them to the options
      self.$('#shopItems').find(".selected").each(function() {
         var item = $( this ) ;
         if (item.attr('isPet') == 'true') {
            self._options.starting_pets.push(item.attr('uri'));
         } else {
            self._options.starting_talismans.push(item.attr('uri'));
         }
      });

      self._options.starting_gold = self.get('gold');

      App.shellView.addView(App.StonehearthSelectSettlementView, {options: self._options});
      self.destroy();
   },

   _buildCitizensArray: function() {
      var self = this;

      var citizenMap = this.get('model.citizens');
      this._citizensArray = radiant.map_to_array(citizenMap, function(citizen_id ,citizen) {
         citizen.set('__id', citizen_id);
         citizen.set('portrait', '/r/get_portrait/?type=headshot&entity=' + citizen.__self);
      });

      self._citizensRoster.stonehearthRoster('updateRoster', self._citizensArray);
   },

   _doBuy: function(item) {
      var self = this;
      var gold = self.get('gold');
      var cost = parseInt(item.attr('cost'));

      if (item.hasClass('selected')) {
         gold -= cost;
      } else {
         gold += cost;
      }

      self.set('gold', gold);
   },

   _doReset: function(quantity) {
      var self = this;
      self.set('gold', self._startingGold);
      self._shopPalette.stonehearthEmbarkShopPalette('updateItems', self._shopItemData);
   },

});
