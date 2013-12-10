$(document).ready(function(){
   $(top).on("radiant_create_stockpile", function (_, e) {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' );
      // xxx, localize
      $(top).trigger('radiant_show_tip', { 
         title : 'Click and drag to create your stockpile',
         description : 'Your citizens place resources and crafted goods in stockpiles for safe keeping!'
      });
      radiant.call('stonehearth:choose_stockpile_location')
         .always(function() {
            radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
            $(top).trigger('radiant_hide_tip');
            console.log('stockpile created!');
            if (e && e.callback) {
               e.callback()
            }
         });
   });
});

