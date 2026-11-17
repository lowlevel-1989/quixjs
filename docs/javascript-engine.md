# The JavaScript engine

English | [Español](javascript-engine.es.md)

quixjs runs on **JavaScriptCore**, the engine of WebKit (Safari, GNOME Web,
WPE), built by us as a static library and used through its C API
(`runtime/js/`, the only place that knows the engine).

**QuickJS was our first choice**, and quixjs ran on it before version 0.2.0:
it is tiny (about 1 MB, 9 MB of memory) and very easy to embed. It was dropped
in favour of JavaScriptCore because it is an interpreter without JIT (it never
turns the JavaScript into machine code), and the games we port (RPG Maker MV)
call thousands of small functions per frame. Even with every optimization we
made ([engine hooks](../runtime/engines/README.md), native helpers in C, a
better profiler), Fear & Hunger did not reach 30 fps on the handhelds. A
benchmark with RPG-Maker-like code on the RG CubeXX (Cortex-A53):

| engine                    | time    | max memory |
|---------------------------|---------|------------|
| QuickJS                   | 2212 ms | 9 MB       |
| JavaScriptCore (JIT)      | 147 ms  | 208 MB     |
| V8 (Node, JIT)            | 107 ms  | 204 MB     |

In the game, on the PC, one update of the heaviest map went from 14-19 ms
(QuickJS) to 3.4 ms (JavaScriptCore), and battles from about 22 fps on the
handheld to a flat 60 on the PC. Without JIT neither JavaScriptCore nor V8
beat QuickJS; the JIT is what matters. We took JavaScriptCore over V8 because
it has a plain C API that fits quixjs (C), it is not tied to Node, and it is
kept in good shape on ARM by the WPE embedded port.
