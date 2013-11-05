//REVIEW COMMENTS: Is there a better place to put this function?
//I just wanted to factor it out of the calls.
function call_server_to_place_item(e) {
   // kick off a request to the client to show the cursor for placing
   // the workshop. The UI is out of the 'create workshop' process after
   // this. All the work is done in the client and server

   radiant.call('stonehearth:choose_place_item_location', e.event_data.full_sized_entity_uri)
      .done(function(o){
         radiant.call('stonehearth:place_item_in_world', e.event_data.self, e.event_data.full_sized_entity_uri, o.location, o.rotation);
      })
      .always(function(o) {
         $(top).trigger('hide_tip.radiant');
      });
};

$(document).ready(function(){
   //Fires when someone clicks the place button on an iconic item in the world
   $(top).on("place_item.stonehearth", function (_, e) {
      $(top).trigger('show_tip.radiant', {
         title : i18n.t('stonehearth:item_placement_title') + " " + e.event_data.item_name,
         description : i18n.t('stonehearth:place_description')
      });
      call_server_to_place_item(e);
   });

   //Fires when someone clicks the move button on a full-sized item in the world
   $(top).on("move_item.stonehearth", function (_, e) {
      $(top).trigger('show_tip.radiant', {
         title : i18n.t('stonehearth:item_movement_title') + " " + e.event_data.item_name,
         description : i18n.t('stonehearth:move_description')
      });
      call_server_to_place_item(e);

   });

   //Fires when someone clicks the "place" button in the UI to bring up the picker
   $(top).on("placement_menu.radiant", function (_, e) {
      radiant.call('stonehearth:get_placable_items_tracker')
         .done(function(response) {
            App.gameView.addView(App.StonehearthPlaceItemView, {
                  uri: response.tracker
               });
         });
   });
});

App.StonehearthPlaceItemView = App.View.extend({
   templateName: 'stonehearthItemPicker',
   modal: false,
   components: {
      'entity_types' : {
         entities : {
            'stonehearth:placeable_item_proxy' : {},
            'item' : {}
         }
      }
   },

   //Whether or not the shift key is currently down
   shifted: false,
   waitingForPlacement: false,
   autoSelectNext: null,

   init: function() {
      this._super();
      var self = this;
      //Track shift while this view is active
      $(document).on('keyup keydown', function(e){
         self.shifted = e.shiftKey;
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

         /*
         if (!self.waitingForPlacement
              && !container.is(e.target) // if the target of the click isn't the container...
              && container.has(e.target).length === 0) // ... nor a descendant of the container
         {
            self.destroy();
         }
         */
      });
   },

   didInsertElement: function() {
      this._super();
      $("#itemPicker")
         .animate({ bottom: 10 }, {duration: 300, easing: 'easeOutCirc'});
      //TODO: Flicker happens because the data_store needs to be distinguished for subsections
      $('.pickable_item').tooltip();
   },

   actions: {
      //Fires whenever the user clicks on an object in the UI
      onSelectItemButton: function(item) {
         this._actions.selectItem.call(this, item, 0);
      },

      close: function() {
         this.destroy();
      },

      //When the user selects an item, call the place API
      //If the player is pressing shift when done, set up to
      //automatically place the next thing if there are more of that type to place
      //Chain is the # of times this has been called on the same item, in a row
      selectItem: function(item, chain) {
         this.waitingForPlacement = true;

         var self = this;

         radiant.call('stonehearth:choose_place_item_location', item.full_sized_entity_uri)
            .done(function(o){
               var item_type = 1;
               radiant.call('stonehearth:place_item_type_in_world', item.entity_uri, item.full_sized_entity_uri, o.location, o.rotation);

               self.waitingForPlacement = false;

               //If we're holding down shift after the item has been placed
               //and there are more items of this type, immediately select the next in line.
               //Experiment: allow placement of N items, where N is the number of items at the time
               //May allow placement of more than existing #s of items, if there are 2 players. Players
               //will just have to create more items to fulfill the orders
               var numItemsRemaining = item.entities.length-1
               if(self.shifted && numItemsRemaining > 0 && numItemsRemaining > chain ) {
                  //On next update, call this function again with item
                  self._actions.selectItem.call(self, item, chain + 1);
                  //self.autoSelectNext = item;
               }
            });
      }
   },

});
