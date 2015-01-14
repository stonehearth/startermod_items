App.StonehearthImmigrationReportDialog = App.StonehearthBaseBulletinDialog.extend({
   templateName: 'immigrationReportBulletinDialog',

   didInsertElement: function() {
      this._super();
      var self = this;

      this.passOrFailElement('food_data', this.$('#immigrationReportBulletinDialog #foodBlock'), 500 );
      this.passOrFailElement('morale_data', this.$('#immigrationReportBulletinDialog #moraleBlock'), 1500 );
      this.passOrFailElement('net_worth_data', this.$('#immigrationReportBulletinDialog #netWorthBlock'), 2500 );
      
      //Add a last beat for the conclusion
      
      setTimeout(function(){
         this.$('#immigrationReportBulletinDialog #conclusionDiv').css('visibility', 'visible')
         this.$('#immigrationReportBulletinDialog #conclusionDiv').pulse();
         if (self.get('model.data.accepted_callback')) {
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:immigration_menu:immigration_positive'} );
         } else {
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:immigration_menu:immigration_negative'} );
         }
      }, 3500);

   },

   passOrFailElement: function (elementName, $targetBlock, delay) {
      var self = this;
      setTimeout( function() {
         
         var targetString = 'model.data.message.' + elementName;
         var $targetElement = $targetBlock.find('.statusDiv');
         var $possessedSpan = $targetBlock.find('.possessedSpan')

         $targetElement.css('visibility', 'visible');

         if (self.get(targetString + '.available') >= self.get(targetString + '.target')) {
            $targetElement.removeClass("failed");
            $targetElement.addClass("passed");
            $possessedSpan.removeClass("failedText");
            $possessedSpan.addClass("successText");
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:immigration_menu:immigration_pass'} );

         } else {
            $targetElement.removeClass("passed");
            $targetElement.addClass("failed");
            $possessedSpan.removeClass("successText");
            $possessedSpan.addClass("failedText");
            radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:immigration_menu:immigration_fail'} );
         }
         $targetElement.pulse();    

         //TODO for DougP: pick some other sound? Whatever you like!
      }, delay);
   }

});
