App.StonehearthCreateCampView = App.View.extend({
	templateName: 'createCamp',

   didInsertElement: function() {
      if (!this.first) {
         /// xxx localize
         /*
         $(top).trigger('show_tip.radiant', {
            title : 'Choose your base camp location',
            description : '',
         });
         */

         this.first = true;         
      }

      this._bounceBanner();
   },

   actions : {
      placeBanner: function () {
         var self = this;
         this._hideBanner();
         radiant.call('stonehearth:choose_camp_location')
            .done(function(o) {
               self._showStockpileHint();
            });
      },

      placeStockpile: function () {
         var self = this;
         $(top).trigger('create_stockpile.radiant', {
            callback : function() {
               self._hideCrate();
               self._finish();
            }
         });
      }
   },

   _bounceBanner: function() {
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
   },

   _hideCrate: function() {
      this._cratePlaced = true
      $('#crate').animate({ 'bottom' : -200 }, 100);
   },

   _showStockpileHint: function() {
      var self = this;
      $('#crate > div').fadeIn('slow', function() {
         self._bounceCrate();
      });
   },

   _finish: function() {
      var self = this;
      $('#createCamp').animate({ 'bottom' : -300 }, 150, function() {
         self.destroy();
      })
   }

});
