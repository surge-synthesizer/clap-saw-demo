# Clap Saw Demo Synth

The Clap Saw Demo Synth is a synth we put together in the week before the
CLAP 1.0 Launch to show the developer community a few things we wanted to
cover in our example base. It serves as an example in the following ways

- It binds a VSTGUI UI to a CLAP plugin without using any frameworks
- It shows polyphonic parameter modulation and note expression support
- It sounds pretty good, with a Saw from the algorithm in Surge's Modern oscillator
  and an SVF filter from cytomic
- It is released under the MIT license

While some folks might actually want to use this as a synth, it really does serve
as a pedagogical exercise more than anything else.

## Building the synth

```shell
git clone https://github.com/surge-synthesizer/clap-saw-demo
cd clap-saw-demo
git submodule update --init --recursive
mkdir ignore
cmake -Bignore/build -DCMAKE_BUILD_TYPE=Release   # or DEBUG or whatever
cmake --build ignore/build
```

and you will get `ignore/build/clap-saw-demo.clap`

## Understanding the code

We tried to make an effort to have the code clean to read with reasonable comments.
The best starting point is probably clap-saw-demo.h if you want to understand the
modulation system or clap-saw-demo-editor.h if you want to understand the VSTGUI bindings.

## Sending a change, fix, or PR

This is an open contribution project! We welcome changes and contributions. If you find a small bug
or modify the cmake file to work on a new OS or fix a comment please just send in a PR.

If you want to do something more, we also welcome that! Probably best to open a github issue to chat first
and also, wouldn't you rather work on surge or shortcircuit? Also please keep a few things in mind

1. This is designed to be a CLAP example, so porting it to AudioUnit or whatever would not be productive. If you
   love the algo and sound, just lift the voice class into a new synth
2. This is designed to be a CLAP example, so adding a hundred new features which would make it an awesome synth
   seems like it would be confusing. Also, if you want hundreds of features, you can download Surge for free from
   this very github project!
3. The UI is not very pleasant. I wrote minimal VSTGUI as an example but it could look better. If you want
   to do this and code it up, go for it!

## A note to Linux users

CLAP works great on linux! We have done loads of our primary CLAP development there.
But VSTGUI on linux is a bit trickier. 

VSTGUI has a global timer and global xcb handle which are stored on a dll-static. 
CLAP has extensions per plugin to map to posixFd and timer callbacks from the host.

So what this means today, at this version, is

1: A *single plugin* session will look like an FD and Timer leak on exit since 
   i make no effort to cleanup (which is crash prone if you do it wrong)
2. A *multi plugin* session will have the UI stop working when you delete the
   *first* plugin which showed its UI

This is obviously not OK, and we see how to fix it (basically juggling that global subscription)
but haven't done it yet. If you want to help get in touch, but we will work on it over the
next week.
