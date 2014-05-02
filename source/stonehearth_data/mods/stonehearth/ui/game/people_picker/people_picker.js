App.StonehearthPeoplePickerView = App.View.extend({
   filter_fn: function(person) {
      return true;
   },
   templateName: 'stonehearthPeoplePicker',
   modal: true,
   components: {
      'citizens' : {
         '*' : {
            'unit_info': {}
         }
      }
   },

   destroy: function() {
      radiant.log.info('destroying the people picker...')
      this._super();
   },

   setProperties: function(options) {
      // Clients can specify an 'additional_components' property that specifies
      // additional data to fetch from the population service.  This should be
      // set once, as soon as we get it--before the call to setProperties, since
      // that is where the 'uri' for the data is set!  Hence, we override here to
      // build the additional components, and then call 'super'.
      var self = this;
      var additional_components = options.additional_components;

      if (additional_components) {
         $.each(additional_components, function(k,v) {
            if(k == "__self" || !additional_components.hasOwnProperty(k)) {
               return;
            }

            self.components['citizens']['*'][k] = v;
         });
      }

      this._super(options);
   },

   init: function() {
      this._super();
      var self = this;
      $(top).on('keyup keydown', function(e){
         if (e.keyCode == 27) {
            //If escape, close window
            self.destroy();
         }
      });
   },

   _buildPeopleArray: function() {
      var vals = [];
      var citizenMap = this.get('context.citizens');
      var self = this;

      if (citizenMap) {
         $.each(citizenMap, function(k ,v) {
            if(k == "__self" || !citizenMap.hasOwnProperty(k)) {
               return;
            }

            if (self.filter_fn(v)) {
               vals.push(v);
            }
         });
      }

      this.set('context.citizensArray', vals);
    }.observes('context.citizens.[]'),


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
