$(document).ready(function(){
   // When we get the show_workshop event, toggle the crafting window
   // for this entity.
   $(top).on("radiant_show_farm", function (_, e) {
      var view = App.gameView.addView(App.StonehearthShowFarmView, { uri: e.entity });
   });
});

// Expects the uri to be an entity with a stonehearth:workshop
// component
App.StonehearthShowStockpileView = App.View.extend({
   templateName: 'stonehearthShowFarm',

   components: {
      "unit_info": {},
      "stonehearth:farmer_field" : {}
   },

   modal: false,

   didInsertElement: function() {
      this._super();
   },
});