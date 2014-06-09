$(document).ready(function(){

   // .uisounds macro for the common ui sounds
   $('body').on( 'click', '.window .title .closeButton', function() {
      var el = $(this).parent().parent();
      var view = null

      // walk up the dom heirarchy until we find the ember view for the close button
      while(el.length > 0 && view == null) {
         view = Ember.View.views[el.attr('id')]
         el = el.parent();
         console.log(el.attr('id'));
      }

      // close the view
      if (view != null) {
         view.destroy();
      }
   });
});
