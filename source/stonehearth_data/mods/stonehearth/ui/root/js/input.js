$(document).ready(function(){

   $('body').on( 'focus', 'input', function() {
      radiant.call('stonehearth:enable_camera_movement', false);
      radiant.keyboard.enableHotkeys(false);
   });      

   $('body').on( 'blur', 'input', function() {
      radiant.call('stonehearth:enable_camera_movement', true)
      radiant.keyboard.enableHotkeys(true);
   });      
});
