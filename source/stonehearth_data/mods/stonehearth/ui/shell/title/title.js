App.StonehearthTitleScreenView = App.View.extend({
   templateName: 'stonehearthTitleScreen',
   i18nNamespace: 'stonehearth',
   components: {},

   init: function() {
      this._super();
   },

   didInsertElement: function() {
      var self = this;

      radiant.call('radiant:client_about_info')
         .done(function(o) {
            self.set('context.productName', o.product_name + ' ' + o.product_version_string + ' (' + o.product_branch + ' ' +  o.product_build_number + ')');
            self._populateAboutDetails(o);
         });

      radiant.call('radiant:get_collection_status')
         .done(function(o) {
            if (!o.has_expressed_preference) {
               self.get('parentView').addView(App.StonehearthAnalyticsOptView)
            }
         }); 

      $('#about').click(function(e) {
         $('#aboutDetails').position({
              of: $( "#about" ),
              my: "right bottom",
              at: "right top-10"
            })
            .fadeIn();
      });

      $(document).keyup(function(e) {
         if (e.keyCode == 27) { // esc
            $('#titlescreen').show();
         }   
      });

      $('#radiant').fadeIn(800);
      setTimeout(function() {
         $('#titlescreen').fadeIn(800);
      }, 3000)
   },

   actions: {
      newGame: function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:embark' );
         App.shellView.addView(App.StonehearthLoadingScreenView);
         //this.get('parentView').addView(App.StonehearthNewGameView)
      },

      loadGame: function() {
         App.gotoGame();
      },

      exit: function() {
         radiant.call('radiant:exit');
      },

      credits: function() {

      }
   },

   _populateAboutDetails: function(o) {
      var window = $('#aboutDetails');

      window.html('<table>');

      for (var property in o) {
         if (o.hasOwnProperty(property)) {
            window.append('<tr><td>' + property + '<td>' + o[property])      
         }
      }

      window.append('</table>');
   }
});
