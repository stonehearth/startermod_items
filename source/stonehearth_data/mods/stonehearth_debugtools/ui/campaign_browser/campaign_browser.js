$(document).on('stonehearthReady', function(){
   App.debugDock.addToDock(App.StonehearthCampaignBrowserIcon);
});

App.StonehearthCampaignBrowserIcon = App.View.extend({
   templateName: 'campaignBrowserIcon',
   classNames: ['debugDockIcon'],

   didInsertElement: function() {
      this.$().click(function() {
         App.debugView.addView(App.StonehearthGameMasterView)   
      })
   }
});

var D3Node = SimpleClass.extend({
   init : function (container, uri) {
      var self = this;

      // bookkeeping variables...
      self._uri = uri;
      self._tree = container;
      self._children = [];
      self._visible = true;

      // d3 defaults for where to find tree data
      self.name = 'loading...';

      // trace the root of the tree.
      this._trace = new RadiantTrace(uri)
         .progress(function(o) {
            self._update(o);
         })
         .fail(function(o) {
            console.log('failed to trace game master node', o)
         })
   },

   toggle: function() {
      var self = this;
      self._visible = !self._visible;
      self._tree._update(self);
   },

   inspect: function() {
      var self = this;
      self._tree.options.inspect_node({
         uri: self._uri,
         left: self.y, 
         top: self.x,
      });
   },

   _update : function(ctx) {
      var self = this;

      // updat d3 options...
      self.name = self._tree.options.get_node_name(ctx);

      // update child nodes...
      var child_nodes = self._tree.options.get_node_children(ctx);
      $.each(child_nodes, function(_, uri) {
         var found = false;
         $.each(self._children, function(i, child_node) {
            found = found || (child_node._uri == uri);
         });
         self._children.push(new D3Node(self._tree, uri));
         if (self._visible) {
            self._tree._update(self);
         }
      });
      self._tree._update(self);
   },
});

var D3CollapsableTree = SimpleClass.extend({
   init : function(options) {      
      var self = this;
      var width = 1000, height = 500;

      var m = [20, 120, 20, 120];   // margin...
      var w = width  - m[1] - m[3];
      var h = height - m[0] - m[2];

      self.options = options;
      self._width = w;
      self._height = h;
      self._root = new D3Node(self, options.root_node_uri, options);
      self._nextId = 1;

      self._root.x0 = h / 2;
      self._root.y0 = 0;

      self._vis = d3.select(self.options.container).append("svg:svg")
          .attr("width", w + m[1] + m[3])
          .attr("height", h + m[0] + m[2])
          .attr('class', 'tree')
        .append("svg:g")
          .attr("transform", "translate(" + m[3] + "," + m[0] + ")");

      self._tree = d3.layout.tree()
                        .children(function(n) { return n._visible ? n._children : [] })
                        .size([h, w]);

      self._diagonal = d3.svg.diagonal()
                        .projection(function(d) { return [d.y, d.x]; });

      self._update(self._root);
   },

   _update : function(source) {
      var self = this

      var vis = self._vis
      var tree = self._tree
      var root = self._root
      var diagonal = self._diagonal

      var duration = 300;

      // Compute the new tree layout.
      var nodes = tree.nodes(root).reverse();

      // Normalize for fixed-depth.
      nodes.forEach(function(d) { d.y = d.depth * 100; });

      // Update the text of the current node
      if (source.id) {
         vis.selectAll('g.node[nid="' + source.id + '"] text')
               .html(source.name);
      }

      // Update the nodes…
      var node = vis.selectAll("g.node")
         .data(nodes, function(d) { return d.id || (d.id = ++self._nextId); });

      // Enter any new nodes at the parent's previous position.
      var nodeEnter = node.enter().append("svg:g")
         .attr("class", "node")
         .attr("nid", function(d) { return d.id; })
         .attr("transform", function(d) { return "translate(" + source.y0 + "," + source.x0 + ")"; })

      nodeEnter.append("svg:circle")
         .attr("r", 1e-6)
         .on("click", function(d) { d.toggle(); })
         .style("fill", function(d) { return !d._visible ? "lightsteelblue" : "#fff"; });

      nodeEnter.append("svg:text")
         .attr("dy", "1.5em")
         .on("click", function(d) { d.inspect() })
         .text(function(d) { return d.name; })
         .style("fill-opacity", 1e-6);

      // Transition nodes to their new position.
      var nodeUpdate = node.transition()
         .duration(duration)
         .attr("transform", function(d) { return "translate(" + d.y + "," + d.x + ")"; });

      nodeUpdate.select("circle")
         .attr("r", 4.5)
         .style("fill", function(d) { return !d._visible ? "lightsteelblue" : "#fff"; });

      nodeUpdate.select("text")
         .style("fill-opacity", 1);

      // Transition exiting nodes to the parent's new position.
      var nodeExit = node.exit().transition()
         .duration(duration)
         .attr("transform", function(d) { return "translate(" + source.y + "," + source.x + ")"; })
         .remove();

      nodeExit.select("circle")
         .attr("r", 1e-6);

      nodeExit.select("text")
         .style("fill-opacity", 1e-6);

      // Update the links…
      var link = vis.selectAll("path.link")
         .data(tree.links(nodes), function(d) { return d.target.id; });

      // Enter any new links at the parent's previous position.
      link.enter().insert("svg:path", "g")
         .attr("class", "link")
         .attr("d", function(d) {
           var o = {x: source.x0, y: source.y0};
           return diagonal({source: o, target: o});
         })
       .transition()
         .duration(duration)
         .attr("d", diagonal);

      // Transition links to their new position.
      link.transition()
         .duration(duration)
         .attr("d", diagonal);

      // Transition exiting nodes to the parent's new position.
      link.exit().transition()
         .duration(duration)
         .attr("d", function(d) {
           var o = {x: source.x, y: source.y};
           return diagonal({source: o, target: o});
         })
         .remove();

      // Stash the old positions for transition.
      nodes.forEach(function(d) {
         d.x0 = d.x;
         d.y0 = d.y;
      });
   }
});

App.StonehearthGameMasterView = App.View.extend({
   templateName: 'game_master',
   uriProperty: 'model',

   didInsertElement : function() {
      radiant.call_obj('stonehearth.game_master', 'get_root_context_command')
         .done(function(o) {
            self._node_browser = new D3CollapsableTree({
               container: this.$('#game_master')[0],
               root_node_uri: o.result,
               get_node_name : function(node) { return node.node_name },
               get_node_children : function(node) { return node.child_nodes },
               inspect_node: function(info) {

                  App.debugView.addView(App.StonehearthObjectBrowserView, {
                     uri: info.uri,
                     relativeTo: {
                        top: info.top,
                        left: info.left,
                     }
                  })
               }
            })
         })
         .fail(function(o) {
            console.log('call to stonehearth.game_master:get_data() failed.', o)
         });
   },
});
