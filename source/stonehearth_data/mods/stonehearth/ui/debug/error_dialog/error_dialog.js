App.StonehearthErrorDialogView = App.View.extend({
   templateName: 'errorDialog',

   init: function() {
      this._super();
   },

   didInsertElement: function() {

      $('#errorDialog')
        .draggable()
        .position({
          my: "right top",
          at: "right top",
          of: "body",
        });

      //this.set('uri', '/o/stores/tmp/objects/27');
      /*
      this._super();
      var context = this.get('context');
      if (context.file_content) {
        this.editor = ace.edit(this.$('#editor')[0]);
        this.editor.setTheme("ace/theme/monokai");
        var session = this.editor.getSession();
        if (context.file_type) {
          session.setMode("ace/mode/" + context.file_mode);
        }
        if (context.line_number > 0) {
          var annotations = [
            { row: context.line_number - 1,
              text: context.summary,
              type: context.category,
            }
          ];
          var scrollTop = context.line_number - 10;
          if (scrollTop >= 0) {
            session.setScrollTop(scrollTop * this.editor.renderer.lineHeight)
          }
          session.setAnnotations(annotations);
        }
      }
     */
    },

    actions:  {
       close: function() {
          this.destroy();
      }
    }
});
