$(document).ready(function(){
   var build_editor;
   radiant.call('stonehearth:get_client_service', 'build_editor')
      .done(function(e) {
         build_editor = e.result;
      })
      .fail(function(e) {
         console.log('error getting build editor')
         console.dir(e)
      })

   $(top).on("radiant_create_wall", function (_, e) {
      $(top).trigger('radiant_show_tip', { 
         title : 'Click to place wall segments',
         description : 'Hold down SHIFT while clicking to draw connected walls!'
      });
      radiant.call_obj(build_editor, 'place_new_wall')
         .always(function(response) {
            $(top).trigger('radiant_hide_tip');
         });
   });
   $(top).on("radiant_create_room", function (_, e) {
      radiant.call_obj(build_editor, 'create_room');
   });
});
