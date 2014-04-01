App.StonehearthCreateCampView = App.View.extend({
	templateName: 'createCamp',

   didInsertElement: function() {
      this._super();
      if (!this.first) {
         this.first = true;

         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:loading_screen_success' );      

         this._bounceBanner();
         $("#crateCoverLink").hide();
      }
   },

   actions : {
      placeBanner: function () {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:banner_grab' );
         var self = this;
         this._hideBanner();
         radiant.call('stonehearth:choose_camp_location')
         .done(function(o) {
               if (o.result) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:banner_plant' );
                     /*
                     setTimeout( function() {
                        radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:paper_menu' );
                        //self._gotoStockpileStep();
                        self._gotoFinishStep();
                     }, 1000);
*/

                     // prompt the player for their settlement's name
                     App.gameView.addView(App.StonehearthNameCampView, { 
                           position: {
                              my : 'center bottom',
                              at : 'left+' + App.stonehearthClient.mouseX + " " + 'top+' + (App.stonehearthClient.mouseY - 100),
                              of : $(document),
                              collision : 'fit'
                           }
                        });

                     self.destroy();
               } else {
                  self._showBanner();
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:banner_bounce' );
               }
            });
      },

      placeStockpile: function () {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:popup' );
         radiant.call('radiant:play_sound', 'stonehearth:sounds:box_grab' );
         var self = this;
         self._hideCrate();
         App.stonehearthClient.createStockpile()
            .always(function(response) {
               if(response.result) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                  setTimeout( function() {
                     self._gotoFinishStep();
                  }, 1000);
               } else {
                  self._showCrate();
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:box_bounce' );
               }               
            });
      },

      finish: function () {
         this._finish();
      }
   },

   _gotoStockpileStep: function() {
      var self = this
      this._hideScroll('#scroll1', function() {
         setTimeout( function() {
            self._enableStockpileButton()
         }, 200);
      });
   },

   _gotoFinishStep: function() {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:paper_menu' );
      var self = this
      //this._hideScroll('#scroll2');
      this._hideScroll('#scroll1');
   },

   _bounceBanner: function() {
      var self = this
      if (!this._bannerPlaced) {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:banner_bounce' )
         $('#banner').effect( 'bounce', {
            'distance' : 15,
            'times' : 1,
            'duration' : 300
         });

         setTimeout(function(){
            self._bounceBanner();
         },1000)
      }
   },

   _bounceCrate: function() {
      var self = this
      if (!this._cratePlaced) {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:box_bounce' )
         $('#crate').effect( 'bounce', {
            'distance' : 15,
            'times' : 1,
            'duration' : 300
         });

         setTimeout(function(){
            self._bounceCrate();
         },1000)
      }
   },

   _showBanner: function() {
      this._bannerPlaced = false
      $('#banner').animate({ 'bottom' : -22 }, 100);
      $("#bannerCoverLink").show();
   },

   _hideBanner: function() {
      this._bannerPlaced = true
      $('#banner').animate({ 'bottom' : -300 }, 100);
      $("#bannerCoverLink").hide();
   },

   _hideScroll: function(id, callback) {
      $(id).animate({ 'bottom' : -300 }, 200, function() {
         if (callback != null) {
            callback();
         }
      }); 
   },

   _showCrate: function() {
      this._cratePlaced = true
      $('#crate').animate({ 'bottom' : -40 }, 150);
      $("#crateCoverLink").show();
   },

   _hideCrate: function() {
      this._cratePlaced = true
      $('#crate').animate({ 'bottom' : -240 }, 150);
      $("#crateCoverLink").hide();
   },

   _enableStockpileButton: function() {
      var self = this;
      $("#crateCoverLink").show();
      $('#crate > div').fadeIn('slow', function() {
         self._bounceCrate();
      });
   },

   _finish: function() {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:paper_menu' );
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:action_click' );
      var self = this;
      $('#createCamp').animate({ 'bottom' : -300 }, 150, function() {
         App.gameView._addViews(App.gameView.views.complete);
         self.destroy();
      })
   }

});

App.StonehearthNameCampView = App.View.extend({
   templateName: 'nameCamp',

   didInsertElement: function() {
      var self = this;
      this._super();

      this.$('#name').focus();

      this.$('#name').keypress(function (e) {
         if (e.which == 13) {
            $(this).blur();
            self.$('.ok').click();
        }
      });


      this.$('.ok').click(function() {
         App.stonehearthClient.settlementName(self.$('#name').val());
         App.gameView._addViews(App.gameView.views.complete);
         self.destroy();
      }),

      this.$('#nameCamp').pulse();
   },

});
