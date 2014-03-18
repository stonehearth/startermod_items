#!/bin/bash

# move into the stonehearth directory and start the process in the background
# $! returns the pid of the most recently executed program.
cd source/stonehearth_data
../../build/source/stonehearth/RelWithDebInfo/Stonehearth.exe&
stonehearth_pid=$!
echo stonehearth pid is $stonehearth_pid

# wait a bit for it to start up, then echo the log to stdout
sleep 5
tail -f stonehearth.log&
tail_pid=$!

# wait for the stonehearth process to finished.  $? contains the exit code
# of the process waited on by wait.  remember that so we can return it.
wait $stonehearth_pid
stonehearth_exitcode=$?
echo stonehearth exited with code $stonehearth_exitcode

# kill the tail process we started early and return success or failure.
kill $tail_pid
exit $stonehearth_exitcode