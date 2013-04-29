
$(function () {
   radiant.api.poll();
   var editor = CodeMirror.fromTextArea(document.getElementById('trace_code'), {
      lineNumbers: true,
      matchBrackets: true,
      mode: "text/typescript"
   });
   editor.on('change', function (editor, change) {
      traces.traces[editor.trace_id].code = editor.getValue();
      save_traces();
   });
   var output = CodeMirror.fromTextArea(document.getElementById('trace_output'), {
      lineNumbers: true,
      matchBrackets: true,
      mode: "text/typescript"
   });
   var error_log = CodeMirror.fromTextArea(document.getElementById('error_log'), {
      readOnly : true,
   });

   function log(str) {
      log_prefix('**', str);
   }

   function log_prefix(prefix, str) {
      error_log.setValue(error_log.getValue() + "\n" + ' ' + prefix + ' ' + str);
      var height = error_log.getScrollInfo().height;
      error_log.scrollTo(0, height);
   }

   function add_trace_to_gutter(id) {
      $("#all_traces").append('<li trace_id="'+id+'"><a href="#" trace_id="' + id + '">' + traces.traces[id].name + '</a></li>');
   }

   installed_traces = {}
   trace_results = {}
   function load_traces() {
      var saved = localStorage.getItem('traces');
      if (saved) {
         traces = JSON.parse(saved);
      } else {
         traces = {
            selected: 0,
            traces: {},
            nextId: 1
         }
      }
      $.each(traces.traces, function (key, value) {
         add_trace_to_gutter(key);
      });
   }

   function selected_trace() {
      return traces.traces[traces.selected]
   }

   function save_traces() {
      localStorage.setItem('traces', JSON.stringify(traces));
   }

   function show_trace_name(id) {
      var name = traces.traces[id].name;
      var list_selector = "#all_traces a[trace_id=" + id + "]";
      $("#trace_name").html(name).attr('trace_id', id);
      $(list_selector).html(name);
      //$("#all_traces li[trace_id=" + id + "]").attr('class', 'active');
   }

   function show_trace(id)
   {
      $("#all_traces li[trace_id=" + traces.selected + "]").attr('class', '');
      var trace = traces.traces[id];
      traces.selected = id;
      show_trace_name(id);
      editor.trace_id = id;
      editor.setValue(trace.code);
      $("#all_traces li[trace_id=" + traces.selected + "]").attr('class', 'active');

      if (trace_results[id]) {
         output.setValue(trace_results[id]);
      } else {
         output.setValue("no result.");
      }
   }

   function add_trace() {
      var id = traces.nextId;
      traces.nextId += 1;

      traces.traces[id] = {
         id: id,
         name: 'New Trace #' + id,
         code: '',
      };
      save_traces();
      show_trace(id);
      add_trace_to_gutter(id);
   }

   $("#trace_name").on("click", function (evt) {
      var id = $(this).attr('trace_id');
      var name = $(this).html();
      var input = $('<input>').attr('value', name);
      var label = $(this);

      function restore() {
         input.remove();
         show_trace_name(id);
         label.show();
      }
      input.on('keypress', function (e) {
         if (e.keyCode == 13) {
            traces.traces[id].name = $(this).attr('value');
            save_traces();
            restore();
         }
      }).blur(function (evt) {
         restore();
      });

      $(this).after(input);
      input.focus();
      label.hide();
   });

   $("#all_traces").on("click", "a", function (evt) {
      var id = $(this).attr('trace_id');
      show_trace(id);
   });

   $("#add_trace").on("click", function (evt) { add_trace(); });
   $("#create_stockpile").on("click", function (evt) {      
      radiant.api.execute('create_stockpile');
   });

   $("#install_trace").on("click", function (evt) {
      var trace = selected_trace()
      var installedTraceObj = installed_traces[trace.id];
      if (installedTraceObj) {
         installedTraceObj.destroy();
         delete installed_traces[trace.id];
      }

      var traceObj;
      try { 
         traceObj = eval(trace.code);
      } catch (error) {
         output.setValue(error.message);
         return;
      }
      traceObj
         .progress(function (o) {
            var formatted = JSON.stringify(o, null, 3);
            trace_results[trace.id] = formatted;
            if (trace.id == traces.selected) {
               output.setValue(formatted);
            }
         })
         .fail(function (msg) {
            output.setValue(msg);
         });
      installed_traces[trace.id] = traceObj;
   });

   load_traces();
   show_trace(traces.selected)
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
   });
   $(top).on("radiant.dbg.log", function (_, o) {
      log_prefix('--', o);
   });
   error_log.setValue("ready.");
});
