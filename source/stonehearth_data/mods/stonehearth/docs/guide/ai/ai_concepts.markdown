Tbook: guide
section: ai
title: AI

# Concepts

This guide details the various moving parts in the AI System.  For more information on any part, see that specific guide.

## Actions
  
Actions are the fundamental building blocks which drive Entity behavior.  

An Entity's "stonehearth:ai_component" manages the Entity behavior.   It contains an index of all potentially runnable Actions and a Thread object to run them.  The AI Component Thread will pick the most suitable Action at the current moment, run it (synchronously), and spin back around to pick the next Action.  Each Action may be decomposed into simpler actions, which forms a stack of currently running Actions on the AI Component Thread.

Actions come in two flavors: Simple and Compound.

### Simple Actions

Simple Actions implement the AI Action interface to do an extremely trivial task.  Examples include synchronously running an an Effect on an Entity (e.g. a pickup or eat food animation), moving an Entity from one location to another, or doing some kind of computation required by a Compound Action (e.g. finding a path from one entity to another).

Simple Actions should be though of as extremely basic, generic building blocks which can be used to build complex behavior.  By analogy, a Simple Action is like an English sentence with a very simple sentence structure, especially with no contractions or conditionals (e.g. follow this path, run this animation, put this buff on this Entity, etc.) 

### Compound Actions

Compound Actions are composed of Simple Actions using the AI System's create_compound_action method.  The Compound Action basically runs through each Simple Action sequentially and synchronously.  You can think of a Compound Action as being built up of many Simple Actions strung together with "then" statements (e.g. go to this item, then pickup the item, then carry it to this chest, then put it in the chest).

### No Complex Logic

Actions are run synchronously from start to finish, and an Entity can only be doing one Action at any given time.  Do not try to implement complex logic in an Action!  It will only bring you grief and misery.   To implement complex behavior, you should use Threads and Tasks (see below).

## Tasks and Task Groups

A Task is used to track a **request** to run Actions on one or more Entities.  The Task object has methods to:

- Change the number of times an Action should be performed.
- Start, stop, or monitor the lifetime of the request.
- Wait for the request to complete.
- Change the priority of the task relative to its Task Group (see below)
 
A Task Group is used to feed a set of work items (Tasks) to Entities who are eligible to perform them (Workers).  It has methods to

- Add or remove Entities from the set of workers.
- Create new tasks which those workers will try to perform.
- Change the priority of the TaskGroup for the activity it is created to perform.

A TaskGroup can contain any number of Tasks and any number of Workers.  It uses a variant of the Stable Marriage algorithm to attempt to optimally feed Tasks to Workers.  Task assignment is biased toward giving Workers the highest-priority and "cheapest" Tasks.

Tasks and TaskGroups should be used to build higher-level AI primitives.  Usually these fall into two categories: group behavior and sophisticated individual AI behaviors.

### Coordinated, Group Behaviors

Many times you may want to have an Action performed by any member of a group.  This may be something which should be done only once, or done repeatedly.  For example, you would like any member of the Worker class to chop down a tree (but only one, please), and any member of the Worker class to restock a stockpile (as many as possible, please!).

The Task and Task Group provide an easy, flexible way to coordinate this behavior.  You generally want to:

1. Create an Action for the item you want performed.  This is usually a CompoundAction.
2. Find the TaskGroup which contains all the Workers you'd like to perform it.  This may involve creating a new TaskGroup and adding Entities to it.
3. For each work item, call "create_task" on the Task Group, passing in the Activity for your Action.
4. For each Task you create this way, use the Task functions to configure it's lifetime, max workers, etc.

### Sophisticated Individual Behavior

In many cases, running Actions simply isn't sufficient for implementing Entity behavior.  Only one Action can be running at a time.  Furthermore, once an Action has begun running, it is not allowed to block to do significant computation (e.g. additional pathfinding).  Finally, there is no way for an Action to suspend itself to let another Action run and resume later to pick up where it left off.

To handle all these situations, you need to create a new Thread of execution to manage the complicated scenario, and use Tasks and TaskGroups to request Actions get run to carry out the details.

For example, consider a crafter who needs to process is order list.  There are many, many cases where the crafter may need to do different actions based on any number of events.  Consider, for example, what should happen when the crafter is instructed to build a chair that takes 2 pieces of wood.  If we implemented this as a CompoundAction, we would need to precompute a complicated path to take the crafter from his current position, to each piece of wood, and back to the bench.  Furthermore, the Action would take a really long time to complete, during which either the wood the crafter needs would either be unavailable for other uses (which might look weird) or it might mysteriously disappear, forcing our Action to abort.

Instead of using a CompoundAction, we would like to write code that looks like


    function collect_ingredients()
       while not finished do
           gather_next_ingredient()
       end
    end

Where "gather\_next\_ingredient" is a blocking function which grabs exactly one ingredient from the recipe.  One implementation would be to create a taskgroup for the crafter, add a task which does an action to gather just that one ingredient, then wait for it to finish.  In this way, the crafter can do any number of Actions betweeen calls to gather\_next\_ingredient().  We can also look at the result from the Task wait() function to see if the Action actually completed to decide what we should do next.

The process of creating Threads, TaskGroups, and Tasks to implement these scenariors is **greatly** simplified by the Town orchestrator functionality.  See the Town reference for details.
  