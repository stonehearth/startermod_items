App.StonehearthTitleScreenView = App.View.extend(Ember.TargetActionSupport, {
   templateName: 'stonehearthTitleScreen',
   i18nNamespace: 'stonehearth',
   components: {},

   _compatibleVersions: {
      "0.1.0.375" : true,
      "0.1.0.393" : true,
   },

   init: function() {
      this._super();
   },

   didInsertElement: function() {
      var self = this;

      radiant.call('radiant:client_about_info')
         .done(function(o) {
            self.set('context.productName', o.product_name + ' ' + o.product_version_string + ' (' + o.product_branch + ' ' +  o.product_build_number + ') ' + o.architecture + ' build');
            self._populateAboutDetails(o);
         });


      $.get('/stonehearth/release_notes/release_notes.html')
         .done(function(result) {
            self.set('context.releaseNotes', result);
         })

      // show the load game button if there are saves
      radiant.call("radiant:client:get_save_games")
         .done(function(json) {
            var vals = [];

            $.each(json, function(k ,v) {
               if(k != "__self" && json.hasOwnProperty(k)) {
                  v['key'] = k;
                  vals.push(v);
               }
            });

            // sort by creation time
            vals.sort(function(a, b){
               var tsA = a.gameinfo.timestamp ? a.gameinfo.timestamp : 0;
               var tsB = b.gameinfo.timestamp ? b.gameinfo.timestamp : 0;
               // sort most recent games first
               return tsB - tsA;
            });


            if (vals.length > 0) {
               var save = vals[0];
               var version = save.gameinfo.version;
               if (!version || version != App.stonehearthVersion) {
                  // SUPER hack save game compatibility code so we can push a build
                  // to Steam without real save game compatibility - tonyc
                  if (!self._compatibleVersions[version]) {
                     // For now, just blindly warn if versions are different.
                     save.gameinfo.differentVersions = true;
                  }
               }

               self.$('#continue').show();
               self.$('#continueGameButton').show();
               self.$('#loadGameButton').show();
               
               self.set('context.lastSave', save);
               /*
               var ss = vals[0].screenshot;
               $('#titlescreen').css({
                     background: 'url(' + ss + ')'
                  });
               */
            }
         });


      // load the about info
      $('#about').click(function(e) {
         self.$('#blog').toggle();
      });

      // input handlers
      $(document).keyup(function(e) {
         $('#titlescreen').show();
      });

      $(document).click(function(e) {
         $('#titlescreen').show();
         self._showAlphaScreen();
      });

      $('#radiant').fadeIn(800);
      
      setTimeout(function() {
         $('#titlescreen').fadeIn(800, function() {
            self._showAlphaScreen();
            /*
            radiant.call('radiant:get_collection_status')
               .done(function(o) {
                  if (!o.has_expressed_preference) {
                     self.get('parentView').addView(App.StonehearthAnalyticsOptView)
                  }
               }); 
            */

         });
      }, 3000)
   },

   _showAlphaScreen: function() {
      if (this._alphaScreenShown) {
         return;
      }

      this._alphaScreenShown = true;
      radiant.call('radiant:get_config', 'alpha_welcome')
         .done(function(o) {
            if (!o.alpha_welcome || !o.alpha_welcome.hide) {
               App.shellView.addView(App.StonehearthConfirmView, 
                  { 
                     title : "Welcome to Stonehearth!",
                     message : "Warning: this is Alpha quality software. It's missing many features, and you can expect to encounter bugs as you play.<br><br>You can help by automatically sending us crash reports for your game. Would you like to do this?",
                     buttons : [
                        { 
                           label: "Yes, send crash reports",
                           click: function () {
                              radiant.call('radiant:set_collection_status', true)
                              radiant.call('radiant:set_config', 'alpha_welcome', { hide: true});
                           }
                        },
                        { 
                           label: "Nope",
                           click: function () {
                              radiant.call('radiant:set_collection_status', false)
                              radiant.call('radiant:set_config', 'alpha_welcome', { hide: true});
                           }
                        }                              
                     ] 
                  });
            }
         })

   },

   actions: {
      newGame: function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:embark'} );
         App.shellView.addView(App.StonehearthNewGameOptions);
      },

      continueGame: function() {
         var key = String(this.get('context.lastSave').key);

         // throw up a loading screen. when the game is loaded the browser is refreshed,
         // so we don't need to worry about removing the loading screen, ever.
         radiant.call("radiant:client:load_game", key);
         // At this point, we just wait to be killed by the client.
      },

      loadGame: function() {
         this.triggerAction({
            action:'openModal',
            actionContext: ['save',
               {
                  allowSaves: false,
               }
            ]            
         });
      },

      // xxx, holy cow refactor this together with the usual flow
      quickStart: function() {
         var self = this;
         var MAX_INT32 = 2147483647;
         var seed = Math.floor(Math.random() * (MAX_INT32+1));

         var width =12;
         var height = 8;
         this.$().hide();
         
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:embark'} );
         App.shellView.addView(App.StonehearthLoadingScreenView);

         radiant.call('stonehearth:new_game', width, height, seed, {enable_enemies : true})
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

               radiant.call('stonehearth:generate_start_location', x, y, e.map_info);
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

               
            })
            .fail(function(e) {
               console.error('new_game failed:', e)
            });
      },

      exit: function() {
         radiant.call('radiant:exit');
      },

      credits: function() {

      },
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
