# F-UI1 — FRONTEND_STANDALONE target does not build (compile-stage failure, pre-link)

Severity: MAJOR (blocks the standalone dev workflow the CMake option advertises)
Pin: develop @ e3a7a73 | Evidence: evidencia/ui-standalone-configure.log, ui-standalone-build.log

## Outcome
Configure OK (exit 0, Ninja generated). Build FAILED exit 1 at COMPILE stage — earlier than the
anticipated link stage:

```
/workspace/src/frontend/src/views/teacher_list_widget.cpp:3:10: fatal error:
    middleware/internalclient.h: No such file or directory
    3 | #include <middleware/internalclient.h>
/workspace/src/frontend/src/views/main_window.cpp:18:10: fatal error:
    middleware/internalclient.h: No such file or directory
   18 | #include <middleware/internalclient.h>
ninja: build stopped: subcommand failed.
```

## Root cause
src/frontend/CMakeLists.txt standalone branch (:42-60) finds only Qt6 Core/Widgets/Network and
sets include dirs to frontend's own `include/` + `src/` — it never adds the middleware include
path nor links the middleware library, while three view TUs hard-include
`<middleware/internalclient.h>` and reference `Middleware::OP_*` constants.

## Reproduce (in gestor-dev container)
```
cmake -S /workspace/src/frontend -B /workspace/build-ui -G Ninja -DFRONTEND_STANDALONE=ON
cmake --build /workspace/build-ui     # -> fails as above; build-ui removed post-evidence per task 8.6
```

## Consequence / blast radius
- No user-runnable GUI binary exists in this tree other than the integrated one, which exits right
  after login (see ui-probe-verdicts.md) — i.e. TODAY no flow reaches MainWindow.
- Fix sketch (S): add `target_include_directories(... src/middleware/include)` +
  `target_link_libraries(... middleware)` (or build middleware sources into the standalone target)
  in the standalone branch. Effort: S. Blast radius: 1 file (src/frontend/CMakeLists.txt), no
  product-code coupling changes.

Valid finding per design AD2 — recorded, not a blocker for reporting.
