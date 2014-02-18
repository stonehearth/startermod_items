App.StonehearthCreateCampView = App.View.extend({
	templateName: 'createCamp',

   didInsertElement: function() {
      if (!this.first) {
         /// xxx localize
         /*
         $(top).trigger('radiant_show_tip', {
            title : 'Choose your base camp location',
            description : '',
         });
         */

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
                     setTimeout( function() {
                        radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:paper_menu' );
                        self._gotoStockpileStep();
                     }, 1000);
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
         $(top).trigger('radiant_create_stockpile', {
            callback : function(response) {
               if(response.result) {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:place_structure' );
                  setTimeout( function() {
                     self._gotoFinishStep();
                  }, 1000);
               } else {
                  self._showCrate();
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:box_bounce' );
               }
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
      this._hideScroll('#scroll2');
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
