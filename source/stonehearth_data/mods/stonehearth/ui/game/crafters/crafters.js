App.StonehearthCraftersView = App.StonehearthCitizensView.extend({
   init: function() {
      this._super();

      this.set('title', 'Crafters & jobals');
   },

   _citizenFilterFn: function (citizen) {
      // xxx, need to do a lot better than this. Combat units pass this test.
      return citizen['stonehearth:job']['job_uri']['alias'] != 'stonehearth:jobs:worker';
   },
});
