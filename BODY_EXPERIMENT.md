# EVA-VR Body — experimental branch

This branch starts at EVA-VR v0.2.5 commit `5ca5f42`. It builds a separately
installed experiment, `org.eva.pico.body` / **EVA-VR Body**, with the existing
controller and haptic path plus `XR_BD_body_tracking` full-body sampling. The
original `org.eva.pico.input` is not modified by installing this package. The
Java activity retains its original fully qualified class name, so use the full
component name below. The launcher icon is currently shared with EVA-VR.

**Status:** the debug APK was tested on a PICO 4 Ultra with calibrated waist and
ankle trackers and both controllers. The 24-joint stream, guided recording and
read-only simulation panel were observed on the headset. An experimental
headset-to-GMR-to-MuJoCo kinematic pipeline was exercised via eva-mobile; motion
amplitude still needs tuning. This is not validation across firmware versions,
the full range of poses, or real robot operation. GMR and robot control run
outside this application. See `eva-mobile/docs/body-validation-2026-11-09.md`
and `eva-mobile/docs/body-console-puppet-2026-11-09.md` for the scope and evidence.

## Build and install

The original dependencies are Gradle 7.3.3, Android SDK platform 29,
build-tools 30.0.3, NDK 23.2.8568313, Java 17, and the OpenXR headers and
loader bundled in this repository. On macOS/Linux use `./build.sh`; it produces
`app/build/outputs/apk/debug/app-debug.apk`. On Windows use a compatible shell
to run `./gradlew :app:assembleDebug` with `JAVA_HOME` and `ANDROID_HOME` set.
Before first installation inspect the merged manifest, package, signature and
APK hash, and confirm the package is not already used on the headset. Specify
an authorized device serial for every ADB command. Do not uninstall the old app.

```sh
adb -s "$PICO_SERIAL" install app/build/outputs/apk/debug/app-debug.apk
adb -s "$PICO_SERIAL" reverse tcp:43876 tcp:43876
adb -s "$PICO_SERIAL" shell am start -n org.eva.pico.body/org.eva.pico.input.MainActivity
```

Only use `install -r` later to update the same experiment package after checking
its signing identity. Pair and calibrate Motion Trackers through the device's
supported system workflow. The app enumerates the extension and system
capability and shows `BODY` status; extension presence does not prove the tracker
is paired, calibrated, authorized, or producing correct data. Unsupported APIs,
tracker creation failures, loss of focus and locate failures do not produce
invented joints. This prototype does not request or diagnose body-tracking
permissions; behavior on other firmware remains to be established.

## Read-only observation

Create a project-local environment with uv and run the monitor on the computer:

```sh
uv venv --python 3.11
uv pip install --python .venv 'websockets==12.0'
uv run --no-project python tools/body_monitor.py --show-joints
```

Then configure ADB reverse and launch the experiment as above. The monitor binds
only to localhost by default, accepts `/ws?token=eva`, and does not send robot
commands. `--show-joints` prints the raw Body object; omit it for a compact
valid-joint count. Avoid running it on a port already used by EVA. The code also
accepts a `server_url` activity extra as in the original application.

The corresponding experimental EVA branch forwards `body` in its normalized
frame; older receiver versions may drop it. A connected host status alone is
only a WebSocket connection, not evidence that Body reached EVA or GMR.

The experimental dashboard also offers a read-only guided observation: when
the host is connected and BODY shows valid 24/24, press left Y. An eight-second
preparation countdown is followed by 12 seconds each for standing, moving the
left arm, moving the right arm, bending and squatting. Focus loss interrupts the
guide; press Y again after tracking recovers. Raw frames carry `body_guide`
phase/run/countdown metadata. Recording is performed by the PC bridge, not by
the button, so start the bridge with recording enabled first. The guide does not
arm or control a robot.

For a third-person **2-D** view of the local MuJoCo node in this same app, run
`eva-mobile/examples/hardware/gento_luna/headset_view.py` on the computer and
forward its localhost port with `adb -s "$PICO_SERIAL" reverse tcp:43877
tcp:43877`. Start the simulation node and the usual input bridge separately.
The panel shows `SIMULATION VIEW` when recent frames arrive and reports a
disconnect after frames become stale. This view is read-only, does not start or
authorize robot control, and does not provide stereo or calibrated XR placement.

## Provisional wire data

The existing version-1 controller frame includes a provisional `body` object.
It is intentionally **not a frozen EVA/GMR contract**. Successful locates carry
24 entries in the bundled `XrBodyJointBD` enumeration order, each with `name`,
raw `location_flags`, `position` and `orientation_xyzw`. An invalid or non-finite
position/orientation is `null`; an orientation with near-zero norm is also
invalid. An unsuccessful locate has an empty joint array.
The runtime aggregate validity flag is reported separately and must not replace
per-joint validation. Units and axes follow OpenXR; values are relative to the
actual OpenXR `baseSpace`, identified by `reference_space` (normally `Local`),
not an assumed ground-aligned or parent-local skeleton. No axis conversion or
retargeting is applied.

`xr_predicted_display_time` is the time requested from OpenXR, **not a measured
hardware capture time**. Body is sampled during rendering and attached to the
next controller frame, so samples are asynchronous. No maximum age, clock
conversion, unique session ID, recenter identifier or firmware version is yet
provided; consumers must not assume synchronized hardware capture times. The
current control integration is experimental and requires separate validation.
Startup/focus-loss status objects may omit sample metadata. The inherited
controller frame labels its reference space `local-floor`; this is legacy
behavior and must not be used to reinterpret the Body `Local` space.

Before freezing a protocol, validate firmware, 24 joint ordering, tracking and
validity flags, frames, units, quaternion conventions, calibration and recenter
behavior against PICO documentation and repeated headset observations. The plan in
`eva-mobile/docs/eva_vr_body_gmr_1_0_plan.md` contains the subsequent stages.

## Windows project-local environment

The `feature/eva-vr-body` workspace uses `.toolchain/jdk-17` and
`.toolchain/android-sdk`. These directories and `.venv` are ignored by Git;
they do not change the machine-wide Java or Android settings. The local Python
3.11 environment was created with `uv` and contains `websockets==12.0`.
Once Android SDK platform 29, build-tools 30.0.3 and NDK 23.2.8568313 have been
installed into that SDK, build from PowerShell:

```powershell
.\tools\build-body.ps1
.\.venv\Scripts\python.exe tools\body_monitor.py --show-joints
```

The script uses the repository's Gradle wrapper and temporarily sets Java and
SDK paths for that process. It does not need to change global environment
variables. An APK build alone is not a headset or tracking test.
