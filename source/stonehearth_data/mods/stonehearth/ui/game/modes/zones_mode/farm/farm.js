App.StonehearthFarmView = App.View.extend({
   templateName: 'stonehearthFarm',

   components: {
      "unit_info": {},
      "stonehearth:farmer_field" : {}
   },


   init: function() {
      var self = this;
      this._super();
      this.set('foo', 'hello world');

      $('#addCropButton').prop('disabled', true);

      //Get the crops available for this farm
      radiant.call('stonehearth:get_all_crops')
         .done(function(o){
            self.set('all_crops', o.all_crops);
            $('#addCropButton').prop('disabled', false);
         });

      //xxx placeholder
      this.set('crop_rotation', []);
   },

   didInsertElement: function() {
      this._super();
      var self = this;

      this.$('#name').focus(function (e) {
         radiant.call('stonehearth:enable_camera_movement', false)
      });

      this.$('#name').blur(function (e) {
         radiant.call('stonehearth:enable_camera_movement', true)
      });

      this.$('#name').keypress(function (e) {
         if (e.which == 13) {
            radiant.call('stonehearth:set_display_name', self.uri, $(this).val());
            $(this).blur();
        }
      });

      this.$('button.warn').click(function() {
         self.destroy();
      });

      this.$('#farmWindow').draggable();

   },

   destroy : function() {
      radiant.call('stonehearth:enable_camera_movement', true);
      this._super();
   },

   addCropToRotation: function(cropId) {
      var crops = this.get('context.crop_rotation') || [];

      crops.push({ crop: cropId});

      this.set('context.crop_rotation', crops);
      console.log(this.get('crop_rotation'));
   },

   actions :  {
      addCropButtonClicked: function() {
         var self = this;
         palette = App.gameView.addView(App.StonehearthFarmCropPalette, { 
                     click: function(item) {
                        var cropId = $(item).attr('crop')
                        self.addCropToRotation($(item).attr('crop'));
                     },
                     context: {
                        items : self.get('all_crops')
                     },
                     position: {
                        my : 'center',
                        at : 'center',
                        of : this.$('#addCropButton')
                     }
                  });
      },
   },
});

App.StonehearthFarmCropPalette = App.View.extend({
   templateName: 'stonehearthCropPalette',
   modal: true,

   init: function() {
      this._super();

   },

   didInsertElement: function() {
      this._super();
      var self = this;

      this.$('.item').click(function() {
         if (self.click) {
            self.click($(this));
         }
         self.destroy();
      });
   }
});