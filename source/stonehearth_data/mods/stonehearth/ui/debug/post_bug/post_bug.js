
// Common functionality between the save and load views
App.StonehearthPostBugView = App.View.extend({
   templateName: 'postBugView',
   classNames: ['flex', 'fullScreen'],
   modal: true,

   didInsertElement: function() {
      var self = this;

      this._super();
      radiant.call('radiant:get_config', 'user_id')
         .done(function(response) {
            self.$('#userId').val(response.user_id);
            setTimeout(function() {
               self.$('#userId').focus();
            }, 100);
         })
      
   },

});
