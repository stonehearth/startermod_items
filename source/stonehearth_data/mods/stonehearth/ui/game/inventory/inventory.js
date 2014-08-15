var StonehearthInventory;

(function () {
   StonehearthInventory = SimpleClass.extend({

      components: {
         "items" : {
            "*" : {
               "unit_info": {},
               "stonehearth:promotion_talisman" : {},               
            }
         }
      },

      init: function(initCompletedDeferred) {
         var self = this;
         this._initCompleteDeferred = initCompletedDeferred;

         return radiant.call('stonehearth:get_inventory')
            .done(function(response) {
               self._inventoryUri = response.inventory;
               self._createTrace();
            })
      },

      _createTrace: function() {
         var self = this;

         var r  = new RadiantTrace()
         var trace = r.traceUri(this._inventoryUri, this.components);
         trace.progress(function(eobj) {
               self._inventory = eobj
               $(top).trigger('inventory_changed', { 
                  inventory : self._inventory
               });
            });

         this._initCompleteDeferred.resolve();
      },

      getData: function() {
         return this._inventory;
      },
   });
})();

