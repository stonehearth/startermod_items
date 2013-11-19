App.StonehearthNewGameView = App.View.extend({
   templateName: 'stonehearthNewGame',
   i18nNamespace: 'stonehearth',

   modal: true,

   init: function() {
      this._super();
      var self = this;

      radiant.call('radiant:get_collection_status')
            .done(function(o) {
               var expressed_opinion = o.has_expressed_preference;
               var collection_status = o.collection_status;
               if (expressed_opinion) {
                  self.set('context.collection_status', collection_status);
               } else {
                  //User hasn't expressed an opnion yet, so default to yes
                  self.set('context.collection_status', true);
               }
            });  
   },

   actions : {
      startNewGame: function() {
         App.shellView.addView(App.StonehearthLoadingScreenView);
         this.destroy();

         radiant.call('radiant:set_collection_status', $('#analyticsCheckbox').is(':checked'));
      }
   }

});
