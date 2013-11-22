$(document).ready(function(){
   $('body').on( 'click', '[clicksound]', function() {
      radiant.call('radiant:play_sound', $(this).attr('clicksound') );
   });

   $('body').on( 'mouseover', '[mouseoversound]', function() {
      radiant.call('radiant:play_sound', $(this).attr('mouseoversound') );
   });
});
