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
      //this.set('crop_rotation', []);
     
   },

   //Temporary
   _setCurrCrop: function() {
      var curr_crop_name = this.get('context.stonehearth:farmer_field.crop_queue')[0].name;
      this.set('context.curr_crop_name', curr_crop_name)
   }.observes('context.stonehearth:farmer_field'),

   didInsertElement: function() {
      this._super();
      var self = this;

      if (this.get('context.stonehearth:farmer_field') == undefined ) {
         return;
      }

      //ar curr_crop_name = this.get('context.stonehearth:farmer_field.crop_queue')[0].name;
      //this.set('context.curr_crop_name', curr_crop_name)

      this.$('#name').keypress(function (e) {
         if (e.which == 13) {
            radiant.call('stonehearth:set_display_name', self.uri, $(this).val());
            $(this).blur();
        }
      });

      this.$('button.warn').click(function() {
         radiant.call('stonehearth:destroy_entity', self.uri)
         self.destroy();
      });

      this.$('button.ok').click(function() {
         radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:start_menu:submenu_select' );
         self.destroy();
      });
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

   //temporary
   change_default_crop: function(cropId) {
      var field = this.get('context.stonehearth:farmer_field').__self;
      radiant.call_obj(field, 'change_default_crop', cropId);
   },

   actions :  {
      addCropButtonClicked: function() {
         var self = this;
         palette = App.gameView.addView(App.StonehearthFarmCropPalette, { 
                     click: function(item) {
                        radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:action_click' );
                        var cropId = $(item).attr('crop');
                        //TODO: add back when we make the queue
                        //self.addCropToRotation($(item).attr('crop'));
                        self.change_default_crop(cropId);
                     },
                     context: {
                        items : self.get('all_crops')
                     },
                     position: {
                        my : 'center',
                        at : 'center', 
                        of : this.$('.targetCrop')
                     }
                  });
      },
   },
});

App.StonehearthFarmCropPalette = App.View.extend({
   templateName: 'stonehearthCropPalette',
   modal: true,

   init: function() {
      radiant.call('radiant:play_sound', 'stonehearth:sounds:ui:action_click' );
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

      this.$('.item').tooltipster();
   }
});