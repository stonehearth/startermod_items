$(document).ready(function(){
   var build_editor;
   radiant.call('stonehearth:get_build_editor')
      .done(function(e) {
         build_editor = e.build_editor;
      })
      .fail(function(e) {
         console.log('error getting build editor')
         console.dir(e)
      })

   $(top).on("create_wall.radiant", function (_, e) {
      radiant.call_obj(build_editor, 'place_new_wall');
   });
   $(top).on("create_room.radiant", function (_, e) {
      radiant.call_obj(build_editor, 'create_room');
   });
});
