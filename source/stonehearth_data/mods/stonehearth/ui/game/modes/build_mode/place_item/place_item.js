$(document).ready(function(){
   //Fires when someone clicks the place button on an iconic item in the world
   $(top).on("radiant_place_item", function (_, e) {
      var item = e.event_data.self

      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' )
      App.stonehearthClient.placeItem(e.event_data.self);
   });
});

App.StonehearthPlaceItemView = App.View.extend({
   templateName: 'stonehearthPlaceItem',
   classNames: ['fullScreen', 'flex', "gui"],
   modal: false,
   components: {
      'entity_types' : {
         entities : {
            'stonehearth:iconic_form' : {},
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
            console.log(e.tracker);
         })
         .fail(function(e) {
            console.log('error getting inventory for player')
            console.dir(e)
         })
   },

   didInsertElement: function() {
      var self = this;

      self.$('.itemButton').click(function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' )

         var itemType = $(this).attr('uri');
         App.stonehearthClient.placeItemType(itemType);
      });
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