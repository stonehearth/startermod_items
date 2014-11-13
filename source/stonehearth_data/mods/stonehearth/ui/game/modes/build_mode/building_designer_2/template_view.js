App.StonehearthTemplateNameView = App.View.extend({
   templateName: 'templateNameView',
   classNames: ['fullScreen', "flex"],

   didInsertElement: function() {
      var self = this;
      this._super();

      this.$('#name').val('binky');
      this.$('#name').focus();

      this.$('#name').keypress(function (e) {
         if (e.which == 13) {
            $(this).blur();
            self.$('.ok').click();
        }
      });

      this.$('.ok').click(function() {
         var templateName = self.$('#name').val()
         //var templateName = templateDisplayName.split(' ').join('_').toLowerCase();

         radiant.call('stonehearth:save_building_template', self.building.__self, { 
            name: templateName
         })

         self.destroy();

      });
   }
});
