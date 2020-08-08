# Support Request
In case you need support or you found a bug it's all about informations that you need to deliver. 

## Usual request
For this purpose and to save a lot of time we included a report creation and upload function into the web configuration.
So visit your web configuration and click on System -> Log. Now you click on the button "Upload report for support request".
> Image of report tool

If everything has gone well, you will notice a Link below the button. Add this link to your support request at our [Hyperion Project Forum](https://forum.hyperion-project.org).

## Segmentation faults
Debugging segmentation faults requires a bunch of work, if we don't own your hardware (idr one of these plenty ARM systems) or can't reconstruct the segmentation fault we need a backtrace log from you. In order to create one, follow these steps.
  - You need a "Debug" version of Hyperion, download and install it over your existing installation.
  - Install "GDB", gbd is a tool which is often used for debugging. Get it from the software repository of your distribution (Debian e.g. `sudo apt-get install gdb`

### Steps of execution
  * Open a terminal
  * Make sure Hyperion is NOT running, this can be done by typing `sudo service hyperiond stop` into the terminal and press enter
  * Type in `gdb` and press enter. You will now see the gdb welcome information and a "(gdb)" in front of your cursor
  * Tell gdb where "hyperiond" is located, usually at /usr/share/hyperion/bin/hyperiond. Prepend "file" to the path. So type into terminal something like that and press enter: `file /usr/share/hyperion/bin/hyperiond`
  * gdb should tell you now that the binary has been loaded with it's symbols etc
  * Now type in `run` and press enter, this will start Hyperion. Now you can use Hyperion as usual, repeat the steps you did to create a segmentation fault.
  * A segmentation fault happened, when Hyperion stops responding and you see something like this as last message at the terminal: `Thread 1 "hyperiond" received signal SIGSEGV, Segmentation fault.`
  * Now type in `backtrace` and press enter, add the backtrace to your support request thread at our forum. [Hyperion Project Forum](https://forum.hyperion-project.org)
  * To quit gdb press enter and type in `quit`, you can start Hyperion again with `sudo service hyperiond start`. It's not recommended to use "Debug" Hyperion builds in production, just install the "Release" version again.

### Example backtrace log
```
  (gdb) backtrace
  #0  0x0000000000000000 in ?? ()
  #1  0x00000000006173f2 in LinearColorSmoothing::queueColors (this=0xfdfa70, 
      ledColors=std::vector of length 34, capacity 34 = {...})
      at /home/hyperion/Dokumente/hyperion.ngBeta/libsrc/hyperion/LinearColorSmoothing.cpp:153
  #2  0x0000000000617374 in LinearColorSmoothing::updateLeds (this=0xfdfa70)
      at /home/hyperion/Dokumente/hyperion.ngBeta/libsrc/hyperion/LinearColorSmoothing.cpp:143
  #3  0x0000000000609652 in LinearColorSmoothing::qt_static_metacall (
      _o=0xfdfa70, _c=QMetaObject::InvokeMetaMethod, _id=1, _a=0x7fffffffd190)
      at /home/hyperion/Dokumente/hyperion.ngBeta/build/libsrc/hyperion/moc_LinearColorSmoothing.cpp:85
  #4  0x00007ffff59abd2a in QMetaObject::activate(QObject*, int, int, void**) ()
     from /usr/lib/x86_64-linux-gnu/libQt5Core.so.5
  #5  0x00007ffff59b85c8 in QTimer::timerEvent(QTimerEvent*) ()
     from /usr/lib/x86_64-linux-gnu/libQt5Core.so.5
  #6  0x00007ffff59acbb3 in QObject::event(QEvent*) ()
     from /usr/lib/x86_64-linux-gnu/libQt5Core.so.5
  #7  0x00007ffff78a505c in QApplicationPrivate::notify_helper(QObject*, QEvent*)
     () from /usr/lib/x86_64-linux-gnu/libQt5Widgets.so.5
  #8  0x00007ffff78aa516 in QApplication::notify(QObject*, QEvent*) ()
     from /usr/lib/x86_64-linux-gnu/libQt5Widgets.so.5
  #9  0x00007ffff597d38b in QCoreApplication::notifyInternal(QObject*, QEvent*)
  ---Type <return> to continue, or q <return> to quit---
      () from /usr/lib/x86_64-linux-gnu/libQt5Core.so.5
  #10 0x00007ffff59d25ed in QTimerInfoList::activateTimers() ()
     from /usr/lib/x86_64-linux-gnu/libQt5Core.so.5
  #11 0x00007ffff59d2af1 in ?? () from /usr/lib/x86_64-linux-gnu/libQt5Core.so.5
  #12 0x00007ffff4572127 in g_main_context_dispatch ()
     from /lib/x86_64-linux-gnu/libglib-2.0.so.0
  #13 0x00007ffff4572380 in ?? () from /lib/x86_64-linux-gnu/libglib-2.0.so.0
  #14 0x00007ffff457242c in g_main_context_iteration ()
     from /lib/x86_64-linux-gnu/libglib-2.0.so.0
  #15 0x00007ffff59d37cf in QEventDispatcherGlib::processEvents(QFlags<QEventLoop::ProcessEventsFlag>) () from /usr/lib/x86_64-linux-
  gnu/libQt5Core.so.5
  #16 0x00007ffff597ab4a in QEventLoop::exec(QFlags<QEventLoop::ProcessEventsFlag>) () from /usr/lib/x86_64-linux-gnu/libQt5Core.so.5
  #17 0x00007ffff5982bec in QCoreApplication::exec() ()
     from /usr/lib/x86_64-linux-gnu/libQt5Core.so.5
  #18 0x00000000005d9279 in main (argc=1, argv=0x7fffffffde08)
      at /home/hyperion/Dokumente/hyperion.ngBeta/src/hyperiond/main.cpp:337
```

## Report Privacy Policy
Hyperion gathers the following informations and uploads them to our server.
  * System informations (OS, Version, Kernel, Arch) Hyperion runs on.
  * Current state (active components, active priorities, webconfig settings)
  * Browser and OS
  * Configuration file content
  * Log (if available)
No additional data is acquired. We use this informations just for support requests.