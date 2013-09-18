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
      'entity_types' : {
         entities : {
            'stonehearth_items:placeable_item_proxy' : {},
            'item' : {}
         }
      }
   },

   //Whether or not the shift key is currently down
   shifted: false,
   waitingForPlacement: false,
   autoSelectNext: null,
   keepWindowAround: false,

   init: function() {
      this._super();
      var self = this;
      //Track shift while this view is active
      $(document).on('keyup keydown', function(e){
         self.shifted = e.shiftKey
         if (e.keyCode == 27) {
            //If escape, close window
            self.destroy();
            //TODO: cancel the placement in process?
         }
      });
      //If the user clicks outside the window, destroy the window
      //TODO: rework with modal, keep track of stuff separate from view.
      $(document).mouseup(function (e){
         var container = $('#itemPicker');

         if (!self.waitingForPlacement
              && !container.is(e.target) // if the target of the click isn't the container...
              && container.has(e.target).length === 0) // ... nor a descendant of the container
         {
            self.destroy();
         }
      });
   },

   didInsertElement: function() {
      this._super();
      //TODO: animate in from elsewhere
      //TODO: find out why there is this flicker for no-entities
   },

   actions: {
      //Fires whenever the user clicks on an object in the UI
      onSelectItemButton: function(item) {
         this._actions.selectItem.call(this, item);
         //If the user is pressing shift while they click, keep the window open
         if (this.shifted) {
            this.keepWindowAround = true;
         } else {
            $('#itemPicker').hide();
         }
      },

      //When the user selects an item, call the place API
      //If the player is pressing shift when done, set up to
      //automatically place the next thing if there are more of that type to place
      selectItem: function(item) {
         this.waitingForPlacement = true;

         var self = this;
         var placementModUri = '/modules/client/stonehearth_items/place_item';
         var proxy = item.entities[0]['stonehearth_items:placeable_item_proxy'];

         $.get(placementModUri, {
               proxy_entity_id: proxy.entity_id,
               target_mod : proxy.full_sized_entity_mod,
               target_name : proxy.full_sized_entity_name
            })
            .done(function(o){
               self.waitingForPlacement = false;

               //If we're holding down shift after the item has been placed
               //and there are more items of this type, immediately select the next in line.
               if(self.shifted && item.entities.length-1 > 0) {
                  //On next update, call this function again with item
                  self.autoSelectNext = item;
               } else {
                  //If we hadn't selected to keep the window around, now destroy it
                  if(!self.keepWindowAround) {
                     self.destroy();
                  }
               }
               self.keepWindowAround = false;
            });
      }
   },

   putDownMore: function() {
      //fires whenever an entity is added/removed
      if (this.autoSelectNext != null) {
         //select the item that matches the autoSelectNext identifier
         //Unfortunately, this requires iterating through all the items
         //because we need the latest, newest, updated version,
         //not the copy from the last time.
         var item_identifier = this.autoSelectNext.entity_identifier;
         for (var j=0; j<this.get('context.entity_types.length'); j++) {
            var entity_type = this.get('context.entity_types')[j];
            if(entity_type.entity_identifier == item_identifier) {
               this._actions.selectItem.call(this, entity_type);
            }
         }
         this.autoSelectNext = null;
      }
   }.observes('context.entity_types.entities'),

});