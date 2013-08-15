$(document).ready(function(){
   $(top).on("create_stockpile.radiant", function (_, e) {
      // create progress indicator...
      $.get('/modules/client/stonehearth_inventory/create_stockpile') // modules/client blows!! =(
         .always(function() {
            // destroy progress indicator...
         });
   });
});

