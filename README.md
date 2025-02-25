# Place for work

Work goes in here

DevOld Summary:
-ktimer opens char dev file which runs init function in mytimer registering char dev and creating proc file
  -both use the same file operation structure(might want to play around with this to get different output between a cat /proc/mytimer and a call of read in user space)
    -currently calling cat uses the callback mytimer_proc_show using the seq_read interface (was first registered at file open)
-ktimer sets up async signal handler to run at SIGIO flag from kernel
  -calls mytimer_async in kernel module which sets process in async queue 
-format prints argvs to kernel input buffer and separates action between -l and -s flag
  -both currently just trigger write calls
    - -l just writes -l -s writes the full string
-write function in kernel module copy_from_user and separates into a -l or -s
  -for -l currently just prints active timers from the kernel module
  -for -s parses string and sets timer to time and with message
    -using :
          struct timer_info
      {
          struct timer_list timer;
          char timer_msg[MSG_LEN];
          int active;
      };
    -structure used to hold timer, msg and active status
-if -s in userspace pause is called and process sleeps
  -when timer expires mytimer_handler is called and SIGIO is sent to user space by kill
  -sighandler in user space prints the timer message (not the same "copy" sent to kernel)

TO DO:
-We need to implement the -l flag
    -need to find a way to read timers back to user space 
    -options are proc file read or char dev read
-We need to implement the -r
    -already implemented in last Lab can port over easily
-maybe the -m if timer

