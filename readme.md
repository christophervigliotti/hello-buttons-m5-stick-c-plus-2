# Hello Buttons

Button-state demo for the M5Stick C Plus 2. The state model is separated from
the current board-specific input, display, and audio handling so it can be
extended for other Arduino projects.

## State model

[`include/interactivity_state.h`](include/interactivity_state.h) defines the
pollable `InteractivityState` contract. The global instance is
`interactivityState`; it exposes a typed `state` and two child groups:

- `circleIndicators`: display mode and per-button marker states.
- `textIndicators`: current message and typewriter progress/configuration.
- `beeps`: optional tone profiles for each state.
- `onDoubleLong`: optional callback invoked once when entering `BothLong`.

`src/main.cpp` adapts M5Stick buttons and hardware to this model. For another
project, reuse the header and replace that adapter with the project's inputs,
outputs, and actions. Poll confirmed states for actions rather than tying
them to individual hardware button events. `FrontPending` and `TopPending` are
provisional; wait for a confirmed single, double, long, or combined state
before triggering actions:

```cpp
if (interactivityState.state == InteractivityStateKind::BothLong) {
  // Trigger an action for the combined long-press state.
}
```

This app registers `askResetApp()` as `onDoubleLong`. It opens the
`ResetConfirmation` state and displays `reset app?` with `yes` and `cancel`.
A single tap cycles the selection; a double tap on the same button confirms
it. The options are centered on the bottom row. Confirming Yes waits 350 ms,
then restarts the ESP32 and runs startup calibration again. Cancel returns to
Ready. The same both-button long press
opens this prompt during startup tap calibration; cancel resumes calibration.
Assign `nullptr` to disable the listener or register another callback for a
different project.

## States

At startup, the app separately calibrates Front and Top double taps. It
measures the time from the first tap's release to the second tap's press and
adds 100 ms of tolerance. Gaps over one second or long presses restart that
button's calibration. Holding both buttons for the long-press threshold opens
the reset confirmation prompt. Each calibration times out after ten seconds.
If either step is incomplete, both tap windows use the 300 ms default; otherwise
the results are stored in
`interactivityState.frontDoubleTapWindowMs` and
`interactivityState.topDoubleTapWindowMs` for runtime detection. The long-press
threshold is 900 ms. Calibration instructions stay fixed while recording.
The button instruction (`tap front` or `tap top`) types first, followed by a
centered, normal-size bottom line reading `once then again`. `once` is
highlighted while waiting for the first tap; after it is released, `again` is
highlighted while waiting for the second. There is no blinking. The second
highlight remains for 350 ms after release. Calibration text, the
`helloButtons` title, and its ready dots use a 25 ms character interval.
The red marker represents Top; the green marker represents Front. Ready
markers are periods; active markers are hollow or filled arrows.

| State | Text | Markers | Default beep |
| --- | --- | --- | --- |
| `Ready` | `ready` | Both periods | Silent |
| `FrontPending` | `front?` | Green hollow right arrow | Silent |
| `Front` | `front` | Green hollow right arrow | One 2600 Hz pulse |
| `FrontDouble` | `front double` | Green hollow right arrow | Two 2600 Hz pulses |
| `FrontLong` | `front long` | Green filled right arrow | Two 2600 Hz pulses |
| `TopPending` | `top?` | Red hollow left arrow | Silent |
| `Top` | `top` | Red hollow left arrow | One 2600 Hz pulse |
| `TopDouble` | `top double` | Red hollow left arrow | Two 2600 Hz pulses |
| `TopLong` | `top long` | Red filled left arrow | Two 2600 Hz pulses |
| `Both` | `both` | Each marker reflects its button | One 2600 Hz pulse |
| `BothLong` | `both long` | Both filled arrows | Two 2600 Hz pulses |
| `ResetConfirmation` | `reset app? yes cancel` | Both filled arrows | Silent |

After a short press is released, its state remains pending until the double-tap
window expires. A second short tap that begins within the window resolves to
`FrontDouble` or `TopDouble`; otherwise it resolves to the corresponding
single-tap state. A long press resolves to the long state instead. Pressing
the other button cancels a pending tap; simultaneous holds use `Both` states.
`Both` persists when only one button is released. Entering `BothLong` opens the
reset confirmation prompt; confirming Yes reboots, while Cancel returns to
`Ready`.

## Configuration

- Set `interactivityState.circleIndicators.mode` to `Sticky` to retain marker
  states through release and the countdown, or `Instant` to reset a marker
  immediately when its button is released.
- The calibrated tap windows can be adjusted through
  `interactivityState.frontDoubleTapWindowMs` and
  `interactivityState.topDoubleTapWindowMs`.
- Configure each `interactivityState.beeps` profile with `enabled`,
  `frequencyHz`, `durationMs`, `pulseCount`, and `gapMs`. Pending states are
  silent by default; single taps beep once, double taps and long presses twice.
  Set `enabled` to `false` to silence any state. Repeated presses replay the
  current state's profile.
- Set `interactivityState.textIndicators.characterIntervalMs` to tune typing
  speed. It controls status text, prompt text, and the startup title/dots.
  Each line finishes before the next line begins. Repeated presses restart the
  status animation. The dot countdown waits for tap classification, completed
  typing, and all buttons released; one dot appears every 250 ms, with the
  display returning to `Ready` after one second.

## Build and upload

Requires PlatformIO Core and the M5Unified library dependency. From this folder
in macOS:

```sh
export PATH="$HOME/Library/Python/3.13/bin:$PATH"
pio run
pio run --target upload
```