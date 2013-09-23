App.StonehearthPeoplePickerView = App.View.extend({
   templateName: 'stonehearthPeoplePicker',
   modal: true,
   components: {
      'entities': {
         'unit_info' : {}
      }
   },

   destroy: function() {
      radiant.log.info('destroying the people picker...')
      this._super();
   },

   init: function() {
      this._super();
   },


   didInsertElement: function() {
      //TODO: animate in
      var css = this.get('css');
      if (css) {
         $('#peoplePicker').css(css)
      }

      var title = this.get('title');
      var c = this.get('context');

      if(!this.pulsed) {
         //$('#peoplePicker').pulse();
         this.pulsed = true;
      }
   },

   actions: {
      selectPerson: function(person) {
         this.callback(person);
         this.destroy();
      }
   }

});
