$(document).ready(function(){

   //Fires when someone clicks the place button on an iconic item in the world
   $(top).on("place_item.stonehearth_items", function (_, e) {
      $(top).trigger('show_tip.radiant', {
         title : i18n.t('stonehearth_items:place_title') + " " + e.event_data.item_name,
         description : i18n.t('stonehearth_items:place_description')
      });

      // kick off a request to the client to show the cursor for placing
      // the workshop. The UI is out of the 'create workshop' process after
      // this. All the work is done in the client and server
      //

      var placementModUri = '/modules/client/stonehearth_items/place_item';
      $.get(placementModUri, {proxy_entity_id: e.event_data.entity_id,
                              target_mod : e.event_data.target_mod,
                              target_name : e.event_data.target_name
                             })
         .done(function(o){
         })
         .always(function(o) {
            $(top).trigger('hide_tip.radiant');
         });
   });

   //Fires when someone clicks the "place" button in the UI to bring up the picker
   $(top).on("placement_menu.radiant", function (_, e) {
      var view = App.gameView.addView(App.StonehearthPlaceItemView, {
            uri: '/server/objects/stonehearth_inventory/inventory_tracker'});
   });
});

App.StonehearthPlaceItemView = App.View.extend({
   templateName: 'stonehearthItemPicker',
   modal: false,
   components: {
      'entities': {
         'unit_info' : {},
         'stonehearth_items:placeable_item_proxy' : {}
      }
   },

   init: function() {
      this._super();
   },


   didInsertElement: function() {
      //TODO: animate in
      console.log("Showing picker!!")
   },

   actions: {
      selectItem: function(item) {
         var placementModUri = '/modules/client/stonehearth_items/place_item';
         this.set('context.current', item);
         var target_id =  this.get('context.current.stonehearth_items:placeable_item_proxy.entity_id');
         var target_mod = this.get('context.current.stonehearth_items:placeable_item_proxy.full_sized_entity_mod');
         var target_name = this.get('context.current.stonehearth_items:placeable_item_proxy.full_sized_entity_name');

         $.get(placementModUri, {proxy_entity_id: target_id,
                              target_mod : target_mod,
                              target_name : target_name
                             })
            .done(function(o){
            })
            .always(function(o) {
               //TODO: keep it around if shift-click
               if (!event.shiftKey) {
                  this.destroy();
               }
            });
      }
   }
});