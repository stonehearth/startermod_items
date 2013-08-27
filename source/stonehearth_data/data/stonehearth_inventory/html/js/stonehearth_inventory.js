$(document).ready(function(){
   $(top).on("create_stockpile.radiant", function (_, e) {
      // xxx, localize
      $(top).trigger('show_tip.radiant', { 
         title : 'Drag to create your stockpile',
         description : 'Workers place resources and crafted goods in stockpiles for safe keeping!'
      });
      $.get('/modules/client/stonehearth_inventory/create_stockpile') // modules/client blows!! =(
         .always(function() {
            $(top).trigger('hide_tip.radiant');
            console.log('stockpile created!');
         });
   });
});

