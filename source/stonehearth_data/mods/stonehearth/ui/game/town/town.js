// The view that shows a list of citizens and lets you promote one
App.StonehearthTownView = App.View.extend({
	templateName: 'town',
   classNames: ['flex', 'fullScreen'],

   components: {
      "citizens" : {
         "*" : {
            "stonehearth:profession" : {
               "profession_uri" : {}
            },
            "unit_info": {},
         }
      }
   },

   init: function() {
      var self = this;
      this._super();

      //this.set('context.inventory', stonehearth.inventory);
   },

   didInsertElement: function() {
      var self = this;
      this._super();

   },

   preShow: function() {
      //this.$('#citizenManagerJobs').hide();
   },
});

