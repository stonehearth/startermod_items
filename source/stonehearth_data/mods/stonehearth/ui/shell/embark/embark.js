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

      this._requestedPortraits = [];
      this._citizensArray = [];
   },

   didInsertElement: function() {
      this._super();
      var self = this;
      $('body').on( 'click', '#embarkButton', function() {
         self._embark();
      });

      self.$("#regenerateButton").click(function() {
         self._cancelAllPortraitRequests();

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
         radiant.each(data.starting_talisman, function(i, talismanData) {
            var category = i18n.t('embark_shop_category_talisman');
            var talisman = {
               "display_name" : talismanData.display_name,
               "uri" : talismanData.uri,
               "isPet" : false,
               "icon" : talismanData.icon,
               "cost" : talismanData.cost
            };
            self._shopItemData.push(talisman);
         })

         var modules = App.getModuleData();
         if (modules.kickstarter_pets) {
            radiant.each(data.kickstarter_pets, function(i, petData) {
               var category = i18n.t('embark_shop_category_pets');
               var pet = {
                  "display_name" : i18n.t(petData.display_name),
                  "uri" : petData.uri,
                  "isPet" : true,
                  "icon" : petData.icon,
                  "cost" : petData.cost
               };
               self._shopItemData.push(pet);
            })
         }

         self._shopPalette.stonehearthEmbarkShopPalette('updateItems', self._shopItemData);
      });

      $(document).on('keydown', this._clearSelectionKeyHandler);
   },

   destroy: function() {
      if (this._trace) {
         this._trace.destroy();
      }
      this._cancelAllPortraitRequests();
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

   _requestCitizenPortrait: function(citizen) {
      var self = this;
      var scene = {
         lights : [
            {
               color: [0.9, 0.8, 0.9],
               ambient_color: [0.3, 0.3, 0.3],
               direction: [-45, -45, 0]
            },
         ],
         entity: citizen.__self,
         camera: {
            position: [2, 1, -2],
            look_at: [0,0.5,0]
         }
      };

      var callback = {
         fail: function(e) {
            if (e) {
               // Actual error occurred.
               console.error('generate portrait failed:', e)
            }
         },
         success: function(response) {
            citizen.set('portrait', 'data:image/png;base64,' + response.bytes);
            self._citizensRoster.stonehearthRoster('updateRoster', self._citizensArray);
         }
      }

      self._requestedPortraits.push(App.portraitManager.requestPortrait(scene, callback));
   },

   _cancelAllPortraitRequests: function() {
      for (var i=0; i<this._requestedPortraits.length; ++i) {
         App.portraitManager.cancelRequest(this._requestedPortraits[i]);
      }

      this._requestedPortraits.length = 0;
   },

   _buildCitizensArray: function() {
      var self = this;

      self._cancelAllPortraitRequests();

      var citizenMap = this.get('model.citizens');
      this._citizensArray = radiant.map_to_array(citizenMap, function(citizen_id ,citizen) {
         citizen.set('__id', citizen_id);
         citizen.set('portrait', '/stonehearth/ui/shell/embark/images/portrait_not_yet_available.png');
         // TODO(yshan): Uncomment this once we fix the bug where citizens
         // are invisible if we generate their portrait on embark screen.
         self._requestCitizenPortrait(citizen);
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
