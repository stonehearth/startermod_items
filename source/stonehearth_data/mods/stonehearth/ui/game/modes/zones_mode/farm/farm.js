App.StonehearthFarmView = App.View.extend({
   templateName: 'stonehearthFarm',
   closeOnEsc: true,

   components: {
      "unit_info": {},
      "stonehearth:farmer_field" : {}
   },

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
         radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:submenu_select'} );
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

   actions :  {
      addCropButtonClicked: function() {
         var self = this;
         palette = App.gameView.addView(App.StonehearthFarmCropPalette, { 
                     field: this.get('context.stonehearth:farmer_field').__self
                  });
      },
   },
});

App.StonehearthFarmCropPalette = App.View.extend({
   templateName: 'stonehearthCropPalette',
   modal: true,

   init: function() {
      var self = this;
      this._super();
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:action_click'} );
      
      //Get the crops available for this farm
      radiant.call('stonehearth:get_all_crops')
         .done(function(o){
            self.set('crops', o.all_crops);
         });
   },

   didInsertElement: function() {
      this._super();
      var self = this;

      this.$().on( 'click', '[crop]', function() {
         var cropId = $(this).attr('crop');
         if (cropId) {
            //TODO: add back when we make the queue
            //self.addCropToRotation($(item).attr('crop'));
            radiant.call_obj(self.field, 'change_default_crop', cropId);
         }
         self.destroy();
      });

     this.$('.item').each(function() {
         $(this).tooltipster({
            content: $('<div class=title>' + $(this).attr('title') + '</div>' + 
                       '<div class=description>' + $(this).attr('description') + '</div>')
         });
     });
   }
});