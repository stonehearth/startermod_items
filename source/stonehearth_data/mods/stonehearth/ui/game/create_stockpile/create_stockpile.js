$(document).ready(function(){
   $(top).on("create_stockpile.radiant", function (_, e) {
      // xxx, localize
      $(top).trigger('show_tip.radiant', { 
         title : 'Drag to create your stockpile',
         description : 'Workers place resources and crafted goods in stockpiles for safe keeping!'
      });
      radiant.call('stonehearth.choose_stockpile_location')
         .always(function() {
            $(top).trigger('hide_tip.radiant');
            console.log('stockpile created!');
         });
   });
});

