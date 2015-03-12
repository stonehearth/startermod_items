$(document).ready(function(){
   $(top).on("radiant_promote_to_job", function (_, e) {

      App.gameView.addView(App.StonehearthPromotionTree, { 
         citizen: e.entity
      });
   });
});

App.StonehearthPromotionTree = App.View.extend({
   templateName: 'promotionTree',
   classNames: ['flex', 'fullScreen'],
   closeOnEsc: true,

   didInsertElement: function() {
      var self = this;

      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:promotion_menu:scroll_open' });
      this._super();

      var self = this;

      var components = {
         "jobs" : {
            "*" : {
               "description" : {} 
            }
         }
      };

      self._jobsTrace = new StonehearthDataTrace('stonehearth:jobs:index', components);
      self._jobsTrace.progress(function(eobj) {
            self._jobsTrace.destroy();
            self._jobData = eobj.jobs;
            self._getCitizenData();
         });
   },

   // transform the job data map into a tree for use by D3
   _buildTreeData: function() {
      var self = this;
      var root;

      var nodeMap = {};

      // for all jobs in the map
      $.each(self._jobData, function(key, job) {
         if (job && job.description) {
            //lookup the node. build it if necessary
            var node = nodeMap[key] || {};
            nodeMap[key] = node;

            self._buildTreeNode(node, job);

            if (job.description.parent_job) {
               var parentNode = nodeMap[job.description.parent_job] || {};
               nodeMap[job.description.parent_job] = parentNode;

               // add this node as a child
               parentNode.children = parentNode.children || [];
               parentNode.children.push(node);
            } else {
               root = node;
            }            
         }
      });

      return root;
   },

   _buildTreeNode: function(node, job) {
      node.name = job.description.name;
      node.description = job.description.description;
      node.icon = job.description.icon;
      node.alias = job.description.alias;
      node.available = job.available;
   },

   _buildTree: function(treeData) {
      var self = this;

      var margin = {top: 100, right: 50, bottom: 100, left: 50},
          width = 1200 - margin.left - margin.right,
          height = 600 - margin.top - margin.bottom;

      var tree = d3.layout.tree()
          .separation(function(a, b) { return a.parent === b.parent ? 1 : 1.2; })
          .children(function(d) { return d.children; })
          .size([width, height]);

      var svg = d3.select(self.$('#content')[0])
          .append("svg")
          .attr("width", width + margin.left + margin.right)
          .attr("height", height + margin.top + margin.bottom)
         .append("g")
          .attr("transform", "translate(" + margin.left + "," + margin.top + ")");

      var nodes = tree.nodes(treeData);
        
      var node = svg.selectAll(".node")
          .data(nodes)
          .enter()
          .append("g")
          .attr('id', function(d) { return d.alias })
          .attr('class', 'node')
          .on("click", function(d) {
            self._updateUi(d.alias);
            //d3.select(this)
               //.attr('class', 'selected');
         });

      node.append('image')
         .attr('xlink:href', function(d) { 
            return d.available ? '/stonehearth/ui/game/promotion_tree/images/jobButton.png' : '/stonehearth/ui/game/promotion_tree/images/jobButtonUnavailable.png'})
         .attr("x", function(d) { return d.x - 37; })
         .attr("y", function(d) { return height - d.y - 37; })
         .attr('width', 74)
         .attr('height', 74);

      node.append('image')
         //.attr('xlink:href', 'images/jobButton.png')
         //.attr('xlink:href', '/stonehearth/ui/game/promotion_tree/images/jobButton.png')
         .attr('xlink:href', function(d) { return d.icon })
         .attr('class', function(d) { return d.available ? "available" : "unavailable" })
         .attr("x", function(d) { return d.x - 37; })
         .attr("y", function(d) { return height - d.y - 37; })
         .attr('width', 74)
         .attr('height', 74);


      /* some example code, cause d3 is a strange beast
      node.append("rect")
          .attr("width", 140)
          .attr("height", 80)
          .attr("fill", "tan")
          .attr("x", function(d) { return d.x - 70; })
          .attr("y", function(d) { return height - d.y - 40; });

      node.append("text")
          .attr("font-size", "16px")
          .attr("fill", "black")
          .attr("x", function(d) { return d.x; })
          .attr("y", function(d) { return height - d.y - 15; })
          .style("text-anchor", "middle")
          .text(function(d) { return d.name; });

      node.append("text")
          .attr("font-size", "12px")
          .attr("x", function(d) { return d.x; })
          .attr("y", function(d) { return 10 + height - d.y; })
          .style("text-anchor", "middle")
          .text(function(d) { return d.born + "–" + d.died; });

      node.append("text")
          .attr("font-size", "11px")
          .attr("x", function(d) { return d.x; })
          .attr("y", function(d) { return 28 + height - d.y; })
          .style("text-anchor", "middle")
          .text(function(d) { return d.location; });
       */

      var nodeUpdate = node.transition()
         .duration(250);

      nodeUpdate.select('image')
         .attr('class', function(d) { return d.name == self._selectedName ? 'selected' : ''; })
         .text(function(d) { 
            return d.name == self._selectedName ? 'selected' : d.name; 
         });

      var link = svg.selectAll(".link")
          .data(tree.links(nodes))
         .enter()
          .insert("path", "g")
          .attr("fill", "none")
          .attr("stroke", "#fff")
          .attr("stroke-width", 2)
          .attr("shape-rendering", "crispEdges")
          .attr("d", function(d, i) {
                return     "M" + d.source.x + "," + (height - d.source.y)
                         + "V" + (height - (3*d.source.y + 4*d.target.y)/7)
                         + "H" + d.target.x
                         + "V" + (height - d.target.y);
            });
        
      //self.$('svg').draggable();
   },

   _getCitizenData: function(jobData) {
      var self = this;
      var citizenId = this.get('citizen');

      // get the info for the citizen
      self._citizenTrace = new StonehearthDataTrace(citizenId, 
         { 
            'stonehearth:job' : {
               'job_controllers' : {
                  '*' : {}
               }
            },
            'unit_info' : {},
         });
      
      // finally, build the tree
      self._citizenTrace.progress(function(o) {
            self._startingJob = o['stonehearth:job'].job_uri;
            self._citizenJobData = o['stonehearth:job'].job_controllers;
            self._getTalismanData();
            self.set('citizen', o);
            self._citizenTrace.destroy();               
         })
   },

   _getTalismanData: function() {
      var self = this;
      // for each job, determine if it's available based on the tools that are
      // in the world
      self.set('selectedJobAlias', self._startingJob);

      radiant.call('stonehearth:get_talismans_in_explored_region')
         .done(function(o) {
            $.each(o.available_jobs, function(key, jobAlias) {
               //Only add this if the talisman is in the world AND if the reqirements are met
               var requirementsMet = self._calculateRequirementsMet(jobAlias);
               if (requirementsMet) {
                  self._jobData[jobAlias].available = true;
               }
            })

            //The worker job is always available
            self._jobData['stonehearth:jobs:worker'].available = true;

            var treeData = self._buildTreeData(self._jobData);
            self._buildTree(treeData);
            self._addHandlers();
            self._updateUi(self._startingJob);
         });  
   },

   _addHandlers: function() {
      var self = this;

      self.$('#approveStamper').click(function() {
         self._animateStamper(); 
         self._promote(self.get('selectedJobAlias'));
      })
   },

   _updateUi: function(jobAlias) {
      var self = this;
      var jobEl = $(document.getElementById(jobAlias)); // use getElementById because jobAlias contains colons

      // move the selection cursor
      if (!self._jobCursor) {
         self._jobCursor = $('<div>')
            .addClass('cursor')
            .appendTo(self.$('#content'));
      }

      var imgEl = $(jobEl.find('image')[0]);
      var x = parseInt(imgEl.attr('x'));
      var y = parseInt(imgEl.attr('y'));
      $(self._jobCursor).css( 
         {
            top: y + 100, // margin top
            left: x + 50, // margin left
         });

      // tell handlebars about changes
      self.set('selectedJobAlias', jobAlias);

      var job = self._jobData[jobAlias].description;
      self.set('selectedJob', job);

      if (jobAlias == self._startingJob) {
         self.$('#scroll').hide();
      } else {
         self.$('#scroll').show();
      }

      //Need to also check if the class requires another class as a pre-req
      //For example: if the parent job is NOT worker, we need to be level 3 at that job in order to allow upgrade
      var requirementsMet = self._jobData[jobAlias].available || jobAlias == self._startingJob;
      var promoteOk = jobAlias != self._startingJob && requirementsMet;

      if (requirementsMet) {
         self.$('#deniedStamp').hide();
      } else {
         self.$('#deniedStamp').show();
      }

      if (promoteOk) {
         self.$('#approveStamper').fadeIn();
      } else {
         self.$('#approveStamper').fadeOut();
      }

      self.set('requirementsMet', requirementsMet);
      self.set('promoteOk', promoteOk);
   },

   //Call only with jobs whose talismans exist in the world
   //True if the current job is worker or has a parent that is the worker class
   //If there is a parent job and a required level of the parent job,
   //take that into consideration also
   _calculateRequirementsMet: function(jobAlias) {
      var self = this;
      var requirementsMet = false;

      var jobDescription = self._jobData[jobAlias].description;
      var selectedJobAlias = self.get('selectedJobAlias');
      var selectedJob = self._jobData[selectedJobAlias].description;

      if (jobAlias == 'stonehearth:jobs:worker' || jobDescription.parent_job == 'stonehearth:jobs:worker') {
         return true;
      }

      if (jobDescription.parent_job != undefined) {
         var parentJobController = self._citizenJobData[jobDescription.parent_job];
         var parentRequiredLevel = jobDescription.parent_level_requirement;
         
         if (parentJobController != undefined && parentJobController != "stonehearth:jobs:worker" && parentRequiredLevel > 0) {
            $.each(self._citizenJobData, function(jobUri, jobData) {
               if (jobUri == jobDescription.parent_job && jobData.last_gained_lv >= parentRequiredLevel) {
                  requirementsMet = true;
                  return requirementsMet;
               }
            })
         }
      }
      return requirementsMet;
   },

   _promote: function(jobAlias) {
      var self = this;

      var jobInfo = self._jobData[jobAlias];

      var citizen = this.get('citizen');
      var talisman = jobInfo.description.talisman_uri;

      radiant.call('stonehearth:grab_promotion_talisman', citizen.__self, talisman);
   },

   _animateStamper: function() {
      var self = this;

      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:promotion_menu:stamp'});

      // animate down
      self.$('#approveStamper').animate({ bottom: 20 }, 130 , function() {
         self.$('#approvedStamp').show();
         //animate up
         $(this)
            .delay(200)
            .animate({ bottom: 200 }, 150, function () {
               // close the wizard after a short delay
               setTimeout(function() {
                  self.invokeDestroy();
               }, 1500);
            });
         });      
   },

   dateString: function() {
      var dateObject = App.gameView.getDate();
      var date;
      if (dateObject) {
         date = dateObject.date;
      } else {
         date = "Ooops, clock's broken."
      }
      return date;
   },

   destroy: function() {
      radiant.call('radiant:play_sound', {'track' : 'stonehearth:sounds:ui:start_menu:page_down'} );

      if (this._jobsTrace) {
         this._jobsTrace.destroy();
      }

      if (this._citizenTrace) {
         this._citizenTrace.destroy();
      }

      this._super();
   },


});