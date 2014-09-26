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
            }
         };

      var shopUri = this.get('context.data.shop');
      self.shopTrace = new StonehearthDataTrace(shopUri, components);

      self.shopTrace.progress(function(eobj) {
            //if (!self.get('inventoryArray')) {
               var array = self._getInventoryArray(eobj.inventory);
               self.set('inventoryArray', array);
            //} else {
               //self._updateInventoryArray(eobj);
            //}
         });

   }.observes('context.data.shop.inventory'),

   didInsertElement: function() {
      var self = this;

      self._super();

      self.$().on('click', '.row', function() {        
         self.$('.row').removeClass('selected');
         $(this).addClass('selected');
         self._selectedUri = $(this).attr('uri');

         self.$('#buy1Button').removeClass('disabled');
         self.$('#buy10Button').removeClass('disabled');
      });

      self.$('#buy1Button').click(function() {
         var shop = self.get('context.data.shop');
         var item = self.$('.row.selected').attr('uri')
         radiant.call_obj(shop, 'buy_item_command', item)
            .done(function() {
               self._getGold();
            })
      })

      if (self._selectedUri) {
         self.$('[uri=' + self._selectedUri + ']').addClass('selected');
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
