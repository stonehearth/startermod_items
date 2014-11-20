$(document).ready(function(){
   $(top).on("radiant_place_item", function (_, e) {
      var item = e.event_data.self

      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:popup'} )
      App.stonehearthClient.placeItem(e.event_data.self);
   });

   $(top).on("radiant_undeploy_item", function (_, e) {
      var item = e.event_data.self

      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:popup'} )
      App.stonehearthClient.undeployItem(e.event_data.self);
   });
});

App.StonehearthPlaceItemView = App.View.extend({
   templateName: 'stonehearthPlaceItem',
   classNames: ['fullScreen', 'flex', "gui"],
   modal: false,
});

App.StonehearthPlaceItemPaletteView = App.View.extend({
   templateName: 'stonehearthPlaceItemPalette',

   didInsertElement: function() {
      var self = this;
      this._super();
      //this.hide();

      if (this._trace) {
         return;
      }

      // build the palette
      this._palette = this.$('#items').stonehearthItemPalette({
         click: function(item) {
            var itemType = item.attr('uri');
            App.stonehearthClient.placeItemType(itemType);            
         }
      });

      return radiant.call_obj('stonehearth.inventory', 'get_item_tracker_command', 'stonehearth:placeable_item_inventory_tracker')
         .done(function(response) {
            self._trace = new StonehearthDataTrace(response.tracker, {})
               .progress(function(response) {
                  //var itemPaletteView = self._getClosestEmberView(self.$('.itemPalette'));
                  //itemPaletteView.updateItems(response.tracking_data);
                  self._palette.stonehearthItemPalette('updateItems', response.tracking_data);
               });
         })
         .fail(function(response) {
            console.error(response);
         })     
   },

   destroy: function() {
      if (this._trace) {
         this._trace.destroy();
      }
   },

});
