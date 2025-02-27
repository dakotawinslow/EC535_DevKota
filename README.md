# Place for work

Work goes in here

**DevOld Summary:**

- **ktimer** opens char dev file which runs init function in mytimer, registering char dev and creating proc file.  
  - Both use the same file operation structure (might want to play around with this to get different output between `cat /proc/mytimer` and a call of `read` in user space).  
  - Currently, calling `cat` uses the callback `mytimer_proc_show` using the `seq_read` interface (was first registered at file open).  

- **ktimer** sets up an async signal handler to run at `SIGIO` flag from kernel.  
  - Calls `mytimer_async` in kernel module, which sets the process in the async queue.  

- **Format prints argv to kernel input buffer and separates action between `-l` and `-s` flag.**  
  - Both currently just trigger write calls:  
    - `-l` just writes `-l`  
    - `-s` writes the full string  

- **Write function in kernel module `copy_from_user` and separates into a `-l` or `-s`.**  
  - For `-l`, currently just prints active timers from the kernel module.  
  - For `-s`, parses string and sets timer to time and message, using:  

    ```c
    struct timer_info {
        struct timer_list timer;
        char timer_msg[MSG_LEN];
        int active;
    };
    ```

    - Structure used to hold timer, message, and active status.  

- If `-s` in userspace, `pause()` is called and process sleeps.  
  - When the timer expires, `mytimer_handler` is called and `SIGIO` is sent to user space by `kill()`.  
  - `sighandler` in user space prints the timer message (not the same "copy" sent to kernel).  

## TO DO:
- We need to implement the `-l` flag  
  - Need to find a way to read timers back to user space  
  - Options are proc file read or char dev read  
  - Ended up using char dev read to prevent any conflict with proc file requirement

- We need to implement the `-r`  
  - Already implemented in the last lab, can port over easily  

- Maybe implement `-m` if the timer...  
