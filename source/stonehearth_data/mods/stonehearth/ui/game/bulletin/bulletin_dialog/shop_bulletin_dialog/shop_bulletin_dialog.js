App.StonehearthShopBulletinDialog = App.StonehearthBaseBulletinDialog.extend({
	templateName: 'shopBulletinDialog',

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
      self.inventoryTrace = new StonehearthDataTrace(shopUri, components);

      self.inventoryTrace.progress(function(eobj) {
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
         radiant.call_obj(shop, 'buy_item_command', item);
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
      if (this.inventoryTrace) {
         this.inventoryTrace.destroy();
      }

      this._super();
   },
});
