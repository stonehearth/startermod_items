App.StonehearthElementHighlight = App.View.extend({
   templateName: 'stonehearthElementHighlight',

   init: function() {
      this._super();
   },

   didInsertElement: function() {
      var self = this;

      if (this.elementToHighlight) {
         var parent = $(this.elementToHighlight);
         var highlight = this.$('.elementHighlight');

         highlight
            .attr('title', this.helpText)
            .width(parent.outerWidth())
            .height(parent.outerHeight())
            .offset({ 
                  top: parent.offset().top - 4,
                  left: parent.offset().left - 4
               });

         // nuke the highlight once the parent is clicked.
         parent.click(function() {
            self.destroy();
         })

         if (this.helpText) {
            App.stonehearthClient.showTip(this.helpText);
            //highlight.tooltipster();
         }
      }

      console.log('done!');
   },

   destroy: function() {
      if (this.completeFn) {
         this.completeFn()
      }
      
      this._super();
   }
});