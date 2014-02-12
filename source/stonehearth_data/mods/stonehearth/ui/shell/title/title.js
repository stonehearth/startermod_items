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


      $.get('/stonehearth/release_notes/release_notes.html')
         .done(function(result) {
            self.set('context.releaseNotes', result);
         })

      $('#about').click(function(e) {
         $('#aboutDetails').position({
              of: $( "#about" ),
              my: "right bottom",
              at: "right top-10"
            })
            .fadeIn();
      });

      $(document).keyup(function(e) {
         $('#titlescreen').show();
      });

      $(document).click(function(e) {
         $('#titlescreen').show();
      });

      $('#radiant').fadeIn(800);
      
      setTimeout(function() {
         $('#titlescreen').fadeIn(800, function() {

            radiant.call('radiant:get_collection_status')
               .done(function(o) {
                  if (!o.has_expressed_preference) {
                     self.get('parentView').addView(App.StonehearthAnalyticsOptView)
                  }
               }); 

         });
      }, 3000)
   },

   actions: {
      newGame: function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:embark' );
         App.shellView.addView(App.StonehearthEmbarkView);
         //this.get('parentView').addView(App.StonehearthLoadingScreenView)
         this.$().hide();
      },

      // xxx, holy cow refactor this together with the usual flow
      quickStart: function() {
         var self = this;
         var MAX_INT32 = 2147483647;
         var seed = Math.floor(Math.random() * (MAX_INT32+1));

         var width =12;
         var height = 8;

         radiant.call('stonehearth:new_game', width, height, seed)
            .done(function(e) {
               var map = e.map;

               var x, y;
               // XXX, in the future this will make a server call to
               // get a recommended start location, perhaps with
               // a difficulty selector
               do {
                  x = Math.floor(Math.random() * map[0].length);
                  y = Math.floor(Math.random() * map.length);
               } while (map[y][x].terrain_code.indexOf('plains') != 0);

               radiant.call('stonehearth:generate_start_location', x, y);
               radiant.call('stonehearth:get_world_generation_progress')
                  .done(function(o) {
                     self.trace = radiant.trace(o.tracker)
                        .progress(function(result) {
                           if (result.progrss == 100) {
                              //TODO, put down the camp standard.

                              self.trace.destroy();
                              self.trace = null;

                              self.destroy();
                           }
                        })
                  });

               App.shellView.addView(App.StonehearthLoadingScreenView);
               
            })
            .fail(function(e) {
               console.error('new_game failed:', e)
            });

      },


      loadGame: function() {
         App.gotoGame();
         this.$().hide();
      },

      exit: function() {
         radiant.call('radiant:exit');
      },

      credits: function() {

      },
      settings: function() {
         this.get('parentView').addView(App.StonehearthSettingsView);
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
