App.StonehearthBaseZonesModeView = App.View.extend({

   didInsertElement: function() {
      this._super();
      this._deselectWhenDestroyed = true;
   },

   destroy : function() {
      if (this._deselectWhenDestroyed) {
         radiant.call('stonehearth:select_entity', null);
      }
      this._super();
   },

   destroyWithoutDeselect: function() {
      this._deselectWhenDestroyed = false;
      this.destroy();
   }
 
});
