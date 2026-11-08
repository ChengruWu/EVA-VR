# EVA-VR Body — experimental branch

This branch starts at EVA-VR v0.2.5 commit `5ca5f42`. It builds a separately
installed experiment, `org.eva.pico.body` / **EVA-VR Body**, with the existing
controller and haptic path plus `XR_BD_body_tracking` full-body sampling. The
original `org.eva.pico.input` is not modified by installing this package. The
Java activity retains its original fully qualified class name, so use the full
component name below. The launcher icon is currently shared with EVA-VR.

**Status:** a Windows debug APK has been built locally. Target firmware, tracker
pairing, permission behavior, tracking accuracy and coexistence with controllers
have not been verified on a headset. Neither GMR nor robot control is implemented
here. Use a read-only receiver and do not connect a robot during this experiment.

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
invented joints. Android permission requirements remain to be established on
actual firmware; this prototype does not claim to request or diagnose them.

## Read-only observation

Run the monitor on the computer, with Python and `websockets==12.0` installed:

```sh
python -m pip install 'websockets==12.0'
python tools/body_monitor.py --show-joints
```

Then configure ADB reverse and launch the experiment as above. The monitor binds
only to localhost by default, accepts `/ws?token=eva`, and does not send robot
commands. `--show-joints` prints the raw Body object; omit it for a compact
valid-joint count. Avoid running it on a port already used by EVA. The code also
accepts a `server_url` activity extra as in the original application.

The existing `eva-mobile/examples/input_sources/vr_webxr/node.py` currently
normalizes the controller frame and **drops `body`**. A connected host status is
only a WebSocket connection, not evidence that Body reached EVA or GMR.

## Provisional wire data

The existing version-1 controller frame includes a provisional `body` object.
It is intentionally **not a frozen EVA/GMR contract**. Successful locates carry
24 entries in the bundled `XrBodyJointBD` enumeration order, each with `name`,
raw `location_flags`, `position` and `orientation_xyzw`. An invalid or non-finite
position/orientation is `null`; an unsuccessful locate has an empty joint array.
The runtime aggregate validity flag is reported separately and must not replace
per-joint validation. Units and axes follow OpenXR; values are relative to the
actual OpenXR `baseSpace`, identified by `reference_space` (normally `Local`),
not an assumed ground-aligned or parent-local skeleton. No axis conversion or
retargeting is applied.

`xr_predicted_display_time` is the time requested from OpenXR, **not a measured
hardware capture time**. Body is sampled during rendering and attached to the
next controller frame, so samples are asynchronous. No maximum age, clock
conversion, unique session ID, recenter identifier or firmware version is yet
provided; consumers must not infer synchronization or use it for control.
Startup/focus-loss status objects may omit sample metadata. The inherited
controller frame labels its reference space `local-floor`; this is legacy
behavior and must not be used to reinterpret the Body `Local` space.

Before freezing a protocol, validate firmware, 24 joint ordering, tracking and
validity flags, frames, units, quaternion conventions, calibration and recenter
behavior against PICO documentation and repeated headset observations. Check
whether waist/ankle physical tracker poses and controllers coexist. The plan in
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
