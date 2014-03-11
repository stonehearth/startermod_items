$(document).ready(function(){
   $(top).on("new_field.stonehearth", function (_, e) {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' );
      
      // xxx, localize
      $(top).trigger('radiant_show_tip', { 
         title : 'Click and drag to designate a new field.',
         description : 'Farmers will break ground and plant crops here'
      });

      radiant.call('stonehearth:choose_new_field_location')
         .always(function(response) {
            radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
            $(top).trigger('radiant_hide_tip');
            console.log('new field created!');
            if (e && e.callback) {
               e.callback(response)
            }
         });
   });
});

