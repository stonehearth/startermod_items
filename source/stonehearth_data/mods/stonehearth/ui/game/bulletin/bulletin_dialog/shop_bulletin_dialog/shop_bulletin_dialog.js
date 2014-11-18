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

   didInsertElement: function() {
      var self = this;

      self._super();


      if (this._shopTrace) {
         return;
      }

      // build the inventory palettes
      this._buyPalette = this.$('#buyList').stonehearthItemPalette({
         cssClass: 'shopItem',
         itemAdded: function(itemEl, itemData) {
            itemEl.attr('cost', itemData.cost);
            itemEl.attr('num', itemData.num);

            $('<div>')
               .addClass('cost')
               .html(itemData.cost + 'g')
               .appendTo(itemEl);

         },
         click: function(item) {
            self._updateBuyButtons();
         }
      });


      self.shopTrace = new StonehearthDataTrace(this.get('context.data.shop'), {});

      self.shopTrace.progress(function(eobj) {
            self._buyPalette.stonehearthItemPalette('updateItems', eobj.shop_inventory);
         });


      self.$().on('click', '#sellList .row', function() {        
         self.$('#sellList .row').removeClass('selected');
         var row = $(this);

         row.addClass('selected');
         self._selectedUri = row.attr('uri');

         self._updateSellButton();
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
         if (!$(this).hasClass('disabled')) {
            self._doBuy(1);
         }
      })

      self.$('#buy10Button').click(function() {
         if (!$(this).hasClass('disabled')) {
            self._doBuy(10);
         }
      })      

      self.$('#sell1Button').click(function() {
         if (!$(this).hasClass('disabled')) {
            self._doSell(1);
         }
      })

      self.$('#sell10Button').click(function() {
         if (!$(this).hasClass('disabled')) {
            self._doSell(10);
         }
      })      

   },

   _doBuy: function(quantity) {
      var self = this;
      var shop = self.get('context.data.shop');
      var item = self.$('#buyList').find(".selected").attr('uri');

      radiant.call_obj(shop, 'buy_item_command', item, quantity)
         .always(function() {
            self._getGold();
         })
         .fail(function() {
            // play a 'bonk!' noise or something
         })
   },

   _doSell: function(quantity) {
      var self = this;
      var shop = self.get('context.data.shop');
      var item = self.$('#sellList .row.selected').attr('uri')

      radiant.call_obj(shop, 'sell_item_command', item, quantity)
         .always(function() {
            self._getGold();
         })
         .fail(function() {
            // play a 'bonk!' noise or something
         })
   },

   _updateBuyButtons: function() {
      var self = this;

      var item = self.$('#buyList').find(".selected");

      if (item) {
         // update the buy buttons
         var cost = parseInt(item.attr('cost'));
         var numAvailable = parseInt(item.attr('num'));
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

   _updateSellButton: function() {
      var self = this;

      var row = self.$("[uri='" + self._selectedUri + "']");

      if (row) {
         self.$('#sell1Button').removeClass('disabled');   
         self.$('#sell10Button').removeClass('disabled');   
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
      if (this._shopTrace) {
         this._shopTrace.destroy();
      }

      this._super();
   },
});

