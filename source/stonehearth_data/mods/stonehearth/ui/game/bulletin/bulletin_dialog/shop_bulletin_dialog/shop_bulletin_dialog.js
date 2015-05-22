App.StonehearthShopBulletinDialog = App.StonehearthBaseBulletinDialog.extend({
	templateName: 'shopBulletinDialog',
   components : {
      data : {
         shop : {
            sellable_items : {}
         }
      }
   },

   _updateGold: function() {
      var self = this;
      self._updateBuyButtons();
      self._updateSellButtons();
   }.observes('model.data.shop.gold'),

   didInsertElement: function() {
      var self = this;
      self._super();

      self.$('#buy1Button').tooltipster();
      self.$('#buy10Button').tooltipster();
      // build the inventory palettes
      self._buildBuyPalette();
      self._buildSellPalette();

      self.$().on('click', '#sellList .row', function() {        
         self.$('#sellList .row').removeClass('selected');
         var row = $(this);

         row.addClass('selected');
         //self._selectedUri = row.attr('uri');

         self._updateSellButton();
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

      this.$('#buyTab').show();
   },

   _buildBuyPalette: function() {
      var self = this;

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
   },

   _updateInventory: function() {
      var shop_inventory = this.get('model.data.shop.shop_inventory');
      this._buyPalette.stonehearthItemPalette('updateItems', shop_inventory);
   }.observes('model.data.shop.shop_inventory'),

   _updateSellableItems: function() {
      var sellable_items = this.get('model.data.shop.sellable_items.tracking_data');      
      this._sellPalette.stonehearthItemPalette('updateItems', sellable_items);
   }.observes('model.data.shop.sellable_items'),

   _buildSellPalette: function() {
      var self = this;
      
      this._sellPalette = this.$('#sellList').stonehearthItemPalette({
         cssClass: 'shopItem',
         itemAdded: function(itemEl, itemData) {
            itemEl.attr('cost', itemData.resale );
            itemEl.attr('num', itemData.num);

            $('<div>')
               .addClass('cost')
               .html(itemData.resale + 'g')
               .appendTo(itemEl);

         },
         click: function(item) {
            self._updateSellButtons();
         }
      });
   },

   _doBuy: function(quantity) {
      var self = this;
      var shop = self.get('model.data.shop');
      var item = self.$('#buyList .selected').attr('uri');

      radiant.call_obj(shop, 'buy_item_command', item, quantity)
         .fail(function() {
            // play a 'bonk!' noise or something
         })
   },

   _doSell: function(quantity) {
      var self = this;
      var shop = self.get('model.data.shop');
      var item = self.$('#sellList .selected').attr('uri')

      if (!item) {
         return;
      }

      radiant.call_obj(shop, 'sell_item_command', item, quantity)
         .fail(function() {
            // play a 'bonk!' noise or something
         })
   },

   _updateBuyButtons: function() {
      var self = this;

      var item = self.$('#buyList').find(".selected");
      var cost = parseInt(item.attr('cost'));
      // For some reason, if there's nothing selected
      // item will still be defined, but its cost will be NaN.
      if (item && cost) {
         // update the buy buttons
         var numAvailable = parseInt(item.attr('num'));
         var gold = self.get('model.data.shop.gold');

         if (cost <= gold) {
            self._enableButton('#buy1Button');
            self._enableButton('#buy10Button');
         } else  {
            self._disableButton('#buy1Button', 'stonehearth:shop_not_enough_gold');
            self._disableButton('#buy10Button', 'stonehearth:shop_not_enough_gold');
         }
      } else {
         self._disableButton('#buy1Button');
         self._disableButton('#buy10Button');   
      }
   },

   _disableButton: function(buttonId, tooltipId) {
      // Disable the button with a tooltip if provided.
      self.$(buttonId).addClass('disabled');
      if (tooltipId) {
         self.$(buttonId).tooltipster('content', i18n.t(tooltipId));
         self.$(buttonId).tooltipster('enable');
      } else {
         self.$(buttonId).tooltipster('disable');
      }
   },

   _enableButton: function(buttonId) {
      self.$(buttonId).removeClass('disabled');
      self.$(buttonId).tooltipster('disable');
   },

   _updateSellButtons: function() {
      var self = this;

      var item = self.$('#sellList .selected')

      if (!item || item.length == 0) {
         self.$('#sell1Button').addClass('disabled');   
         self.$('#sell10Button').addClass('disabled');   
      } else {
         self.$('#sell1Button').removeClass('disabled');
         self.$('#sell10Button').removeClass('disabled');   
      }
   },
});

