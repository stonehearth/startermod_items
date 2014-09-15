App.StonehearthElementHighlight = App.View.extend({
   templateName: 'stonehearthElementHighlight',

   init: function() {
      this._super();
   },

   didInsertElement: function() {
      var self = this;

      var parent = $(self.elementToHighlight);

      if (parent.length > 0) {
         var highlight = self.$('.elementHighlight');
         var borderWidth = highlight.css("border-left-width");

         highlight
            .attr('title', self.helpText)
            .width(parent.outerWidth())
            .height(parent.outerHeight())
            .offset({ 
                  top: parent.offset().top - 8,
                  left: parent.offset().left - 8
               });

         // nuke the highlight once the parent is clicked.
         parent.click(function() {
            self.destroy();
         })

         if (self.helpText) {
            App.stonehearthClient.showTip(self.helpText, self.helpDescription);
            //highlight.tooltipster();
         }         
      } else {
         console.error('highlight selector matched 0 elements: ' + self.elementHighlight);
      }
   },

   destroy: function() {
      if (this.completeDeferred) {
         this.completeDeferred.resolve();
      }
      
      this._super();
   }
});