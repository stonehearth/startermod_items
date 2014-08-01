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
      'tracking_data' : {
         '*' : {              // category...    (e.g. Furniture)
            '*' : {           // uri of items.. (e.g. stonehearth:comfy_bed)
               'items' : {    // all the items, keyed by id
                  '*' : {
                     'stonehearth:entity_forms' : {}
                  }
               }
            }
         }
      }
   },

   init: function() {
      this._super();

      var self = this;
      radiant.call('stonehearth:get_placable_items')
         .done(function(e) {
            self.set('uri', e.tracker);
            console.log('place item tracker is', e.tracker);
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
            if (value != undefined) {
               arr.push(value);
            }
         }
      });
      return arr;
   },

   _reformatData: function() {
      var arr = []
      var self = this;

      var map = this.get('context.tracking_data');
      if (map) {
         arr = this._mapToArray(map, function(category, entityMap) {
            var category = {
               'name' : category,
               'items' : self._mapToArray(entityMap, function(uri, info) {
                  var count = 0;
                  $.each(info.items, function(id, item){
                     if (info.items.hasOwnProperty(id)) {
                        var ef = item['stonehearth:entity_forms']
                        if (!ef) {
                           console.log('wtf');
                        }
                        if (ef && !ef.placing_at) {
                           count += 1;
                        }
                     }
                  });
                  if (count == 0) {
                     return undefined;
                  }
                  info.uri = uri;
                  info.count = count;
                  return info;
               })
            }
            return category
         });
      }      

      this.set('context.categories', arr);
    }.observes('context.tracking_data.[]').on('init'),
});
