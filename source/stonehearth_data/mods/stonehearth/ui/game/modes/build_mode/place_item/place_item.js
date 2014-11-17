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
   components: {
      'tracking_data' : {
         '*' : {              // category...    (e.g. Furniture)
            '*' : {           // uri of items.. (e.g. stonehearth:furniture:comfy_bed)
               'items' : {    // all the items, keyed by id
                  '*' : {
                     'stonehearth:entity_forms' : {}
                  }
               }
            }
         }
      }
   },

   didInsertElement: function() {
      var self = this;
      this._super();
      this.hide();

      this.set('tracker', 'stonehearth:placeable_item_inventory_tracker')

      self.$().on('click', '.item', function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:popup'} )

         var itemType = $(this).attr('uri');
         self.$('.item').removeClass('selected');
         $(this).addClass('selected');
         App.stonehearthClient.placeItemType(itemType);
      });

   },
});
