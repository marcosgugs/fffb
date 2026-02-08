# fffb

### macOS fake force feedback plugin

> fork of [eddieavd/fffb](https://github.com/eddieavd/fffb) — huge thanks to the original project for making force feedback on macOS possible in the first place

## what?

### fffb

`fffb` is a plugin for [scs games](https://www.scssoft.com/) which uses the [TelemetrySDK](https://modding.scssoft.com/wiki/Documentation/Engine/SDK/Telemetry) to read truck telemetry data, constructs custom forces and plays them on your logitech wheel

## why?

- logitech GHUB is a broken piece of `    `
- force feedback doesn't work on macOS
- i like how the wheel shakes when i press the gas pedal

## how?

the scs related stuff is ripped from the examples bundled with the TelemetrySDK.
communication with the wheel is facilitated by apple's hid device API and uses logitech's classic ffb protocol.

## features

### 900-degree native mode

the plugin automatically sets the wheel to its native 900-degree operating range during initialization, giving you the full rotation range for truck simulation.

### force feedback effects

`fffb` uses all 4 hardware force effect slots to provide realistic immersion:

| slot          | effect               | description                                                                                                                                                                   |
| ------------- | -------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0 (constant)  | self-aligning torque | uses lateral acceleration telemetry to pull the wheel back toward center when cornering. stronger at speed, reduced on heavy braking to simulate grip loss                    |
| 1 (spring)    | centering spring     | speed-dependent centering force with progressive slope. light at parking speeds, strong on the highway. small dead zone at center to avoid fighting the player                |
| 2 (damper)    | steering resistance  | speed and brake influenced damping. provides realistic weight to the steering that increases with speed. braking adds extra resistance to simulate front axle weight transfer |
| 3 (trapezoid) | road surface texture | activates on off-road surfaces (gravel, dirt). oscillation intensity scales with speed and responds to suspension deflection changes for bump detection                       |

### telemetry channels

the plugin reads the following telemetry data from the game:

- truck speed, RPM, gear
- effective steering, throttle, brake, clutch
- truck orientation (heading, pitch, roll)
- **lateral acceleration** — drives the self-aligning torque effect
- **front wheel suspension deflection** (left + right) — drives bump detection for road surface effects
- **wheel surface substance** (left + right) — detects off-road surfaces

### RPM LEDs

the wheel's RPM indicator LEDs are driven by the engine RPM telemetry, progressively lighting up as RPM increases.

## supported wheels

- Logitech G29 (PS4)
- Logitech G923 (PS)
- other Logitech wheels with the classic FFB protocol may work (untested)

## usage

### prebuilt binaries

binaries are available on the [releases page](https://github.com/marcosgugs/fffb/releases)
simply copy `libfffb.dylib` to the plugin directory
(plugin directory should be next to the game's executable, default for ats would be `~/Library/Application\ Support/Steam/steamapps/common/Euro\ Truck\ Simulator 2/Euro\ Truck\ Simulator 2.app/Contents/MacOS/plugins`)

### building from source

to build `fffb`:

```bash
# clone the repo
git clone https://github.com/marcosgugs/fffb && cd fffb

# create build directory
mkdir build && cd build

# configure and build project
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8

# create plugin directory
## should be in same directory as ets2/ats executable
## this should be the default path
mkdir ~/Library/Application\ Support/Steam/steamapps/common/Euro\ Truck\ Simulator 2/Euro\ Truck\ Simulator 2.app/Contents/MacOS/plugins

# copy plugin to plugin directory
cp libfffb.dylib ~/Library/Application\ Support/Steam/steamapps/common/Euro\ Truck\ Simulator 2/Euro\ Truck\ Simulator 2.app/Contents/MacOS/plugins
```

alternatively, you can use the build script to clean, build and install in one step:

```bash
# build and install for Euro Truck Simulator 2 (default)
./build_and_install.sh

# or explicitly
./build_and_install.sh --ets

# build and install for American Truck Simulator
./build_and_install.sh --ats
```

now you can launch ets2/ats.
upon launch, the wheel should do a calibration run and you'll see the advanced sdk features popup, hit OK.
if the wheel starts turning (similarly to the way it turns when plugged in), wheel initialization was successful and you should be good to go!
if the wheel doesn't turn, you can try reloading the pluggin by running `sdk reinit` in the in-game console.
in case this also doesn't help, feel free to raise an issue and include your `fffb` log file located at `/tmp/fffb.log`

## troubleshooting

- **wheel doesn't calibrate on launch**: try `sdk reinit` in the in-game console
- **forces feel too weak/strong**: force tuning constants are in `include/fffb/force/simulator.hxx` — adjust the gain values in `_update_constant` and the amplitude curves in `_update_spring`
- **check logs**: plugin logs are written to `/tmp/fffb.log`

## disclaimer

should work on any apple silicon mac
since scs requires the binaries to be x86_64, it might work out of the box for older x86_64 macs (untested as of now)
works only with Logitech wheels
i'm just a random dude writing code cause he's pissed at logitech, use at your own risk
