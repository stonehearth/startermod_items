App.StonehearthLevelUpBulletinDialog = App.StonehearthBaseBulletinDialog.extend({
   templateName: 'lvUpBulletinDialog',

   actions: {
      showCharSheet: function() {
         var entity = this.get('model.data.zoom_to_entity')
         
         if (entity) {
            App.stonehearthClient.showCharacterSheet(entity);
         }
      }
   }
});
