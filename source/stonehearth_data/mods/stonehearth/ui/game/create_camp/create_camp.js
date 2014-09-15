App.StonehearthCreateCampView = App.View.extend({
	templateName: 'createCamp',
   classNames: ['flex', 'fullScreen'],

   didInsertElement: function() {
      var self = this;

      this._super();
      if (!this.first) {
         this.first = true;

         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:loading_screen_success'} );      

         this._bounceBanner();
      }

      App.stonehearthClient.showTip('Click the banner to choose your settlement\'s location');

      this.$('#banner').click(function() {
         self._placeBanner();
      })
   },

   _placeBanner: function () {
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:banner_grab'});
      App.stonehearthClient.showTip('Now click somewhere in the world');
      var self = this;
      this._hideBanner();
      radiant.call('stonehearth:choose_camp_location')
         .done(function(o) {
            App.stonehearthClient.hideTip();
            if (o.result) {
               radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:banner_plant'} );
                  // prompt the player for their settlement's name
                  App.gameView.addView(App.StonehearthNameCampView, { 
                        position: {
                           my : 'center bottom',
                           at : 'left+' + App.stonehearthClient.mouseX + " " + 'top+' + (App.stonehearthClient.mouseY - 100),
                           of : $(document),
                           collision : 'fit'
                        }, 
                        townName : o.townName
                     });

                  self.destroy();
            }
         })
         .fail(function() {
            self._showBanner();
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:banner_bounce'} );
         });
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
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:paper_menu'} );
      var self = this
      //this._hideScroll('#scroll2');
      this._hideScroll('#scroll1');
   },

   _bounceBanner: function() {
      //return;
      var self = this
      if (!this._bannerPlaced) {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:banner_bounce'} )
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
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:box_bounce'} )
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
   },

   _hideBanner: function() {
      this._bannerPlaced = true
      this.$('#banner').animate({ 'bottom' : -300 }, 100);
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
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:paper_menu'} );
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_click'} );
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

      this.$('#name').val(this.townName);

      this.$('#name').focus();

      this.$('#name').keypress(function (e) {
         if (e.which == 13) {
            $(this).blur();
            self.$('.ok').click();
        }
      });


      this.$('.ok').click(function() {
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:submenu_select'} );
         var townName = self.$('#name').val()
         App.stonehearthClient.settlementName(townName);
         radiant.call('stonehearth:set_town_name', townName);
         App.gameView._addViews(App.gameView.views.complete);

         // kick off the tutorials
         setTimeout(function() {
            App.stonehearthTutorials.start();
         }, 1000);

         self.destroy();

      }),

      this.$('#nameCamp').pulse();
   },

});
