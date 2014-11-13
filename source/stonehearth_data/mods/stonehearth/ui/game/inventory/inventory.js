
App.StonehearthInventoryView = App.View.extend({
   templateName: 'inventory',
   classNames: ['flex'],
   
   init: function() {
      var self = this;
      this._super();

      return radiant.call_obj('stonehearth.inventory', 'get_item_tracker_command', 'stonehearth:basic_inventory_tracker')
         .done(function(response) {
            self.set('uri', response.tracker);
         })
         .fail(function(response) {
            console.error(response);
         })
   },

   foo: function() {
      var data = this.get('context.tracking_data');
      var arr = radiant.map_to_array(data);
      this.set('items', arr);
   }.observes('context.tracking_data'),

   didInsertElement: function() {
      this._super();
      self.$('.item').tooltipster();
   },

});
