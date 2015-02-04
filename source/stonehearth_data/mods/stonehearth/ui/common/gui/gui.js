$(document).ready(function(){

   // walk up the dom heirarchy until we find the ember view for the close button
   var getViewForElement = function(el) {
      var view = null;
      
      while(el.length > 0 && view == null) {
         view = Ember.View.views[el.attr('id')]
         el = el.parent();
      }

      return view;
   };

   // .uisounds macro for the common ui sounds
   $('body').on( 'click', '.window .title .closeButton', function() {
      var el = $(this).parent().parent();
      var view = getViewForElement(el);

      // close the view
      if (view != null) {
         view.destroy();
      }
   });

   $('body').on( 'click', '.gui .tab', function() {
      var view = getViewForElement($(this));
      var tabPage = $(this).attr('tabPage');

      view.$('.tabPage').hide();
      view.$('.tab').removeClass('active');
      $(this).addClass('active');

      view.$('#' + tabPage).show();
   });

});
