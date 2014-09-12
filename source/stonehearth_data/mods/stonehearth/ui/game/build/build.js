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
});
