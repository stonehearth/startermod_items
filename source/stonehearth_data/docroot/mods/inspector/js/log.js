
$(function () {
   var error_log = CodeMirror.fromTextArea(document.getElementById('error_log'), {
      readOnly : true,
   });
   var event_log = CodeMirror.fromTextArea(document.getElementById('event_log'), {
      readOnly : true,
   });

   function append_to_codeview(codeview, text){
      codeview.setValue(codeview.getValue() + "\n" + text);
      var height = codeview.getScrollInfo().height;
      codeview.scrollTo(0, height);
   }

   function log(str) {
      log_prefix('**', str);
   }
   function log_prefix(prefix, str) {
      var text = ' ' + prefix + ' ' + str;
      append_to_codeview(error_log, text);
   }

   // log all commands we receive from the server
   $(top).on("radiant.dbg.on_cmd", function (_, o) {
      log('sending ' + JSON.stringify(o));
   });
   $(top).on("radiant.dbg.on_cmd_success", function (_, o) {
      var prefix = '  command ';
      if (o.pending_command_id) {
         prefix = '  pending command ' + o.pending_command_id + ' ';
      }
      log(prefix + 'succeeded.');
   });
   $(top).on("radiant.dbg.on_cmd_error", function (_, o) {
      var prefix = '  command ';
      if (o.pending_command_id) {
         prefix = '  pending command ' + o.pending_command_id + ' ';
      }
      log(prefix + 'failed with "' + o.reason + '"');
   });
   $(top).on("radiant.dbg.on_cmd_pending", function (_, o) {
      log('  command is pending ' + o.pending_command_id);
   });
   $(top).on("radiant.events.*", function (_, o) {
      log('received event ' + JSON.stringify(o.type));
      var payload = JSON.stringify(o, null, 2);
      append_to_codeview(event_log, payload);
   });
   $(top).on("radiant.dbg.log", function (_, o) {
      log_prefix('--', o);
   });

   error_log.setValue("ready.");
});
