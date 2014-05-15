App.StonehearthCraftersView = App.StonehearthCitizensView.extend({
   init: function() {
      this._super();

      this.set('title', 'Crafters & Professionals');
   },

   _citizenFilterFn: function (citizen) {
      // xxx, need to do a lot better than this. Combat units pass this test.
      return citizen['stonehearth:profession']['profession_uri']['alias'] != 'stonehearth:professions:worker';
   },
});
