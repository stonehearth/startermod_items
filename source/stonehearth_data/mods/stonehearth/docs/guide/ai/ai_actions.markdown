Tbook: guide
section: ai
title: AI

# Actions #

Actions are the fundamental building blocks which drive Entity behavior.   They handle everything from sleeping in a bed, quaffing a beer at the bar, to running away from Cthulhu.  If you need to add more basic capabilities to what an Entity can do, you're probably going to need to write an Action.

## Overview

All Entities have a pool of Actions which they can choose from.  These Actions, collectively, represent the sum of everything the Entity could ever decide to do.  Each Action in the list performs a very specific task, called an Activity.  An Activity is simply a name / args tuple which describes some specific behavior.  For example, the PickupItem Action header looks something like this:

    PickupItem.does = 'stonehearth:pickup_item',
    PickupItem.args = {
      item = Entity
    }  

Here, the 'does' field in the header contains the name of the Activity and the 'args' field the names and types of all arguments passed to the Action running the Activity.  When actually running the Action, all names and types of the fields in the 'args' header field must match.

The act of picking up an item is actually quite complicated.  First, we need to find a path to an item, then follow that path to reach it, then play an animation and finally move the item onto the entities carrying component.  That's a lot to do!  Each of those steps are actually performed by other Actions in the system, each of which exposes its interface through Activities of their own.  For example, PickupItem uses the RunToEntity action which implements "stonehearth:goto\_entity".  RunToEntity in turn uses actions which implement "stonehearth:find\_path\_to\_entity" and "stonehearth:follow\_path".

In Summary.

   * Actions implement all behaviors for an Entity
   * Actions can be composed of other Actions to implement complex behavior
   * Each Action implements exactly one Activity

## Actions vs. Activities

Now's a good time to describe in detail the distinction between Actions and Activities.  Activities are **what** the Entity should be doing.  Actions are **how** it should be done.  The distinction is importatnt.

Suppose we want an Entity to eat some food (the **what**).  We want to be able to implement many ways of actually doing it (the **how**).  The most straight forward is to walk over to a piece of food in a stockpile, pick it up, and eat it.  You could imagine many more ways, though.  What if the Entity is a magic user and has a conjure food spell?  What about going over to a chest and taking a piece of food out of that to eat it?  Maybe there's a Cheese Golem who can eat his arm?  Each of these could be implemented by a separate Action, all of which do "stonehearth:eat_food".  The AI Component will dynamically pick the best Action for the job based on information supplied by the Actions themselves (see below).  

Conceptually, the sum of the Actions attached to an Entity represent a dynamic graph of nodes where Activities are the edges between nodes. 

 
## Dispatching Actions

The Entity's AI Component is responsible for deciding which Action to run at any given time.  All Actions are run on a Thread owned by the AI Component.  This ensures that at most 1 Action can run on an Entity at any given time, which would lead to some odd results.

The AI Component thread is quite simple.  It chooses an Action which does the "stonehearth:top" Activity from the pool of Actions in the Entity and runs it.  When that Action finishes, it chooses another "stonehearth:top" action and runs it again, until the Entity is destroyed.  Those actions, in turn, will execute Actions to do sub-Activities, etc. etc.  until we reach a base Action which is so trivial that it's implemented in straight code.

At each step in this process, there are likely many Actions which implement an Activity (e.g. 'stonehearth:goto_entity' could be done by walking there, or using a teleportation ring, etc.).  The AI Component uses a special election process to determine which Action gets to run.  Specifically:

   1. All Actions are asked to start_thinking simultaneously.  Start_thinking is the mechanism by which an Action is informed that the AI Component would like to run it.
   2. The Action which returns a result first is immediately run.
   3. If multiple Actions all return a result simultaneously, the one with the highest priority gets run.
   4. If multiple Actions all have the same priority, they are each given a share based on the "weight" field in their header, and a random Action is chosen.

