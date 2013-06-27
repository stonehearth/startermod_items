$(document).ready(function(){
      $(top).on("stonehearth_crafter.show_workshop", function (_, e) {
         console.log('showing crafting ui for entity ' + e.entity)
         App.gameView.add(App.StonehearthCrafterView, { uri: e.entity });
      });
});

// Expects the uri to be an entity with a stonehearth_crafter:workshop 
// component
App.StonehearthCrafterView = App.View.extend({
   templateName: 'stonehearthCrafter',
   components: ['crafter', 'crafter.unit_info'],

   didInsertElement: function() {

   },

});