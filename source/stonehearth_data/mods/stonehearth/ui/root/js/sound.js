$(document).ready(function(){

   // .uisounds macro for the common ui sounds
   $('body').on( 'click', '.uisounds', function() {
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_click'} );
   });

   $('body').on( 'mouseover', '.uisounds', function() {
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_hover'} );
   });

   // for setting individual sounds
   $('body').on( 'click', '[clicksound]', function() {
      radiant.call('radiant:play_sound', {'track': $(this).attr('clicksound')} );
   });

   $('body').on( 'mouseover', '[hoversound]', function() {
      radiant.call('radiant:play_sound', {'track' : $(this).attr('hoversound')} );
   });
});
