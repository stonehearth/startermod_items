App.StonehearthShopBulletinDialog = App.StonehearthBaseBulletinDialog.extend({
	templateName: 'shopBulletinDialog',

   init: function() {
      this._super();
      var self = this;
      return radiant.call('stonehearth:get_inventory')
         .done(function(response) {
            self._inventoryUri = response.inventory;
            self._getGold();
         })
   },

   _getGold: function() {
      var self = this;
      radiant.call_obj(self._inventoryUri, 'get_gold_count_command')
         .done(function(response) {
            self.set('playerGold', response.gold);
            self._updateBuyButtons();
         })
   },

   _expandInventory: function() {
      var self = this

      var components = 
         {
            'inventory' : {
               '*' : {
                  'item' : {
                     'unit_info' : {}
                  }
               }
            },
            'player_inventory' : {
               'tracking_data' : {
                  '*' : {
                     'item' : {
                        'unit_info' : {}
                     }
                  }               
               }
            }
         };

      var shopUri = this.get('context.data.shop');
      self.shopTrace = new StonehearthDataTrace(shopUri, components);

      self.shopTrace.progress(function(eobj) {
            //if (!self.get('inventoryArray')) {
               var inventoryArray = self._getInventoryArray(eobj.inventory);
               self.set('inventoryArray', inventoryArray);

               var playerInventoryArray = self._getInventoryArray(eobj.player_inventory.tracking_data);
               self.set('playerInventoryArray', playerInventoryArray);

            //} else {
               //self._updateInventoryArray(eobj);
            //}
         });

   }.observes('context.data.shop.inventory, context.data.shop.player_inventory.tracking_data'),

   didInsertElement: function() {
      var self = this;

      self._super();

      self.$().on('click', '#buyList .row', function() {        
         self.$('.row').removeClass('selected');
         var row = $(this);

         row.addClass('selected');
         self._selectedUri = row.attr('uri');

         self._updateBuyButtons();
      });

      self.$('#buyButton').click(function() {
         self.$('#buyList').show();
         self.$('#sellList').hide();
         self.$('#shopTitle').html('Shop inventory');
         self.$('#buyButtons').show();
         self.$('#sellButtons').hide();
      });

      self.$('#sellButton').click(function() {
         self.$('#buyList').hide();
         self.$('#sellList').show();
         self.$('#shopTitle').html('My inventory');
         self.$('#buyButtons').hide();
         self.$('#sellButtons').show();
      });

      self.$('#buy1Button').click(function() {
         var button = $(this);
         if (button.hasClass('disabled')) {
            return
         }

         self._doBuy(1);
      })

      self.$('#buy10Button').click(function() {
         var button = $(this);
         if (button.hasClass('disabled')) {
            return
         }

         self._doBuy(10);
      })      
   },

   _doBuy: function(quantity) {
      var self = this;
      var shop = self.get('context.data.shop');
      var item = self.$('#buyList .row.selected').attr('uri')

      radiant.call_obj(shop, 'buy_item_command', item, quantity)
         .always(function() {
            self._getGold();
         })
         .fail(function() {
            // play a 'bonk!' noise or something
         })
   },

   _updateBuyButtons: function() {
      var self = this;

      var row = self.$("[uri='" + self._selectedUri + "']");

      if (row) {
         // update the buy buttons
         var cost = parseInt(row.attr('cost'));
         var numAvailable = parseInt(row.attr('num'));
         var gold = self.get('playerGold');

         if (cost <= gold) {
            self.$('#buy1Button').removeClass('disabled');   
            self.$('#buy10Button').removeClass('disabled');   
         } else {
            self.$('#buy1Button').addClass('disabled');   
            self.$('#buy10Button').addClass('disabled');   
         }
      } else {
         self.$('#buy1Button').addClass('disabled');   
         self.$('#buy10Button').addClass('disabled');   
      }
   },

   _updateInventoryHtml: function() {
      Ember.run.scheduleOnce('afterRender', this, '_selectShopRow')
   }.observes('inventoryArray'),

   _selectShopRow: function() {
      var self = this;
      if (self._selectedUri) {
         var selector = "[uri='" + self._selectedUri + "']";
         self.$(selector)
            .addClass('selected');
      }
   },

   _getInventoryArray: function(map) {
      var vals = [];
      $.each(map, function(k ,v) {
         if(k != "__self" && map.hasOwnProperty(k)) {
            vals.push(v);
         }
      });
      return vals;
   },

   destroy: function() {
      if (this.shopTrace) {
         this.shopTrace.destroy();
      }

      this._super();
   },
});
