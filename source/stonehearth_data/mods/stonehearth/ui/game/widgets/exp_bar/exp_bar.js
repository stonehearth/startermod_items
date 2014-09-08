App.StonehearthExpBarWidget = App.View.extend({
   templateName: 'expBar',
   classNames: ['fullScreen', 'flex'],

   components: {
      "stonehearth:profession" : {
      },
   },

   init: function() {
      var self = this;
      this._super();

      radiant.call('stonehearth:get_town_entity')
         .done(function(response) {
            self.townTrace = new StonehearthDataTrace(response.town, self.components);
            self.townTrace.progress(function(eobj) {
                  console.log(eobj);
                  self.set('context.town', eobj);
               });
         });
   },

   didInsertElement: function() {
      var textDisplay = this.$('#textDisplay');

      this.$('#expBar').hover( function() { 
            textDisplay.fadeIn() 
         }, function() { 
            textDisplay.fadeOut() 
         });
   },

   _setExpBar: function() {
      var self = this;

      var exp = self.get('context.town.stonehearth:profession.current_level_exp')
      var totalLevelExp = self.get('context.town.stonehearth:profession.total_level_exp')
      var expPercent = Math.floor(100 * exp / totalLevelExp);

      self.$('#expBarFull').width(expPercent + '%');

   }.observes('context.town.stonehearth:profession.current_level_exp'),

   destroy: function() {
      if (this.townTrace) {
         this.townTrace.destroy();

      }
      
      this._super();
   },

});

