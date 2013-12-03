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

         //Play music as the game starts
         var args = {
            'track': 'stonehearth:music:world_start',
            'channel' : 'bgm',
            'fade': 500
         };
         radiant.call('radiant:play_music', args);         
          var args = {
               'track': 'stonehearth:ambient:summer_day',
               'channel': 'ambient', 
               'volume' : 40
            };
            radiant.call('radiant:play_music', args);  
         }


      this._bounceBanner();
      $("#crateCoverLink").hide();

   },

   actions : {
      placeBanner: function () {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:banner_grab' );
         var self = this;
         this._hideBanner();
         radiant.call('stonehearth:choose_camp_location')
         .done(function(o) {
            radiant.call('radiant:play_sound', 'stonehearth:sounds:banner_plant' );
               setTimeout( function() {
                  radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:paper_menu' );
                  self._gotoStockpileStep();
               }, 1000);
            });
      },

      placeStockpile: function () {
         var self = this;
         self._hideCrate();
         $(top).trigger('radiant_create_stockpile', {
            callback : function() {
               setTimeout( function() {
                  self._gotoFinishStep();
               }, 1000);
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
      radiant.call('radiant:play_sound', 'stonehearth:sounds:banner_bounce' );
      var self = this
      if (!this._bannerPlaced) {
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
      radiant.call('radiant:play_sound', 'stonehearth:sounds:box_bounce' );
      var self = this
      if (!this._cratePlaced) {
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

   _hideBanner: function() {
      this._bannerPlaced = true
      $('#banner').animate({ 'bottom' : -300 }, 100);
      $("#bannerCoverLink").hide();
   },

   _hideScroll: function(id, callback) {
     $(id).animate({ 'bottom' : -300 }, 200, function() { callback(); }); 
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
