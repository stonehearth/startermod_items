//REVIEW COMMENTS: Is there a better place to put this function?
//I just wanted to factor it out of the calls.
function call_server_to_place_item(e) {
   // kick off a request to the client to show the cursor for placing
   // the workshop. The UI is out of the 'create workshop' process after
   // this. All the work is done in the client and server

   radiant.call('stonehearth:choose_place_item_location', e.event_data.self, e.event_data.full_sized_entity_uri)
      .done(function(o){
         radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' )
      })
      .always(function(o) {
         $(top).trigger('radiant_hide_tip');
      });
};

$(document).ready(function(){
   //Fires when someone clicks the place button on an iconic item in the world
   $(top).on("radiant_place_item", function (_, e) {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' )
      $(top).trigger('radiant_show_tip', {
         title : i18n.t('stonehearth:item_placement_title') + " " + e.event_data.item_name,
         description : i18n.t('stonehearth:item_placement_description')
      });
      call_server_to_place_item(e);
   });

   //Fires when someone clicks the move button on a full-sized item in the world
   $(top).on("radiant_move_item", function (_, e) {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' )
      $(top).trigger('radiant_show_tip', {
         title : i18n.t('stonehearth:item_movement_title') + " " + e.event_data.item_name,
         description : i18n.t('stonehearth:item_movement_description')
      });
      call_server_to_place_item(e);

   });

});

App.StonehearthPlaceItemView = App.View.extend({
   templateName: 'stonehearthPlaceItem',
   classNames: ['fullScreen', 'flex', "gui"],
   modal: false,
   components: {
      'entity_types' : {
         entities : {
            'stonehearth:placeable_item_proxy' : {},
            'item' : {}
         }
      }
   },

   init: function() {
      this._super();

      var self = this;
      radiant.call('stonehearth:get_placable_items')
         .done(function(e) {
            self.set('uri', e.tracker);
         })
         .fail(function(e) {
            console.log('error getting inventory for player')
            console.dir(e)
         })
   },

   _mapToArray : function(map, convert_fn) {
      var arr = [];
      var self = this;

      $.each(map, function(k, v) {
         var value;
         if (k.indexOf('__') != 0 && map.hasOwnProperty(k)) {
            var value = convert_fn(k, v);
            arr.push(value);
         }
      });
      return arr;
   },

   _reformatData: function() {
      var arr = []
      var self = this;

      var map = this.get('context.tracking_data');
      if (map) {
         arr = this._mapToArray(map, function(k, v) {
            var category = {
               'name' : k,
               'items' : self._mapToArray(v, function(k, v) {
                  v['uri'] = k;
                  return v;
               })
            }
            return category
         });
      }      

      this.set('context.categories', arr);
    }.observes('context.tracking_data.[]').on('init'),
});

App.StonehearthPlaceItemViewOld = App.View.extend({
   templateName: 'stonehearthPlaceItem',
   classNames: ['fullScreen', 'flex', "gui"],
   modal: false,
   components: {},

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
      $('.pickable_item').tooltipster();
   },

   actions: {
      //Fires whenever the user clicks on an object in the UI
      onSelectItemButton: function(item) {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:action_click' );
         this._actions.selectItem.call(this, item, 0);
      },

      close: function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:page_down_right' )
         this.destroy();
      },

      //When the user selects an item, call the place API
      //If the player is pressing shift when done, set up to
      //automatically place the next thing if there are more of that type to place
      //Chain is the # of times this has been called on the same item, in a row
      selectItem: function(item, chain) {
         this.waitingForPlacement = true;

         var self = this;

         radiant.call('stonehearth:choose_place_item_location', item.entity_uri, item.full_sized_entity_uri)
            .done(function(o){
               var item_type = 1;
               radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                              
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
