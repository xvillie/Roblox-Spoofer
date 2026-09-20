# Spoofer

A small Windows console utility that changes the MAC address of a selected
network adapter and clears Roblox-related cookies from local storage and
common browsers.

---

## Table of contents

- [Features](#features)
- [Requirements](#requirements)
- [Getting started](#getting-started)
  - [Option A — download the release](#option-a--download-the-release)
  - [Option B — clone and build](#option-b--clone-and-build)
- [Usage](#usage)
- [Project layout](#project-layout)
- [How it works](#how-it-works)
- [Troubleshooting](#troubleshooting)
- [Disclaimer](#disclaimer)
- [License](#license)

---

## Features

- **MAC spoof** — assigns a random locally-administered MAC to a chosen
  physical adapter via the `NetworkAddress` registry value and cycles the
  interface with `netsh`.
- **Roblox client cookie clear** — deletes
  `%LOCALAPPDATA%\Roblox\LocalStorage\RobloxCookies.dat`.
- **Browser cookie clear** — removes `roblox.com` rows from the cookies
  SQLite DBs of Chrome, Edge, Opera, Opera GX, and Firefox (all
  profiles). Terminates the browser processes first so the DBs aren't
  locked.
- **Menu driven** — pick one action or run the full sweep.
- **Self-elevating** — relaunches via UAC if not already admin.

---

## Requirements

- Windows 10 or later
- Administrator privileges at runtime (handled automatically via UAC)

For building from source:

- Visual Studio 2022 with the **Desktop development with C++** workload
- v143 toolset, Windows 10 SDK, `/std:c++23`
- Git (only if you clone rather than download the ZIP)

---

## Getting started

### Option A — download the release

1. Go to the [Releases](../../releases) tab.
2. Download `spoofer.exe` from the latest release.
3. Right-click → **Run as administrator** (or just double-click and
   accept the UAC prompt).

### Option B — clone and build

Clone the repo:

```bash
git clone https://github.com/xvillie/Roblox-Spoofer.git
cd Roblox-Spoofer
```

Open the solution:

```bash
start spoofer.sln
```

In Visual Studio:

1. Select the **Release** configuration and **x64** platform from the
   toolbar dropdowns.
2. **Build → Build Solution** (`Ctrl+Shift+B`).
3. The compiled binary lands at `release\build\spoofer.exe`.

Or build from the command line with MSBuild:

```bash
msbuild spoofer.sln /p:Configuration=Release /p:Platform=x64
```

Run it:

```bash
release\build\spoofer.exe
```

---

## Usage

Launch the exe. If it isn't already elevated it will relaunch itself
through UAC. You'll see:

```
                              [?] Spoofer

  select an option:

  [1] spoof mac address only
  [2] clean browser cookies only (chrome/edge/firefox/opera)
  [3] clean roblox client cookies only
  [4] full  (spoof + clean everything)
  [5] quit

  >
```

Type a number and press Enter.

- Options **2** and **4** kill running browser processes before touching
  their cookie DBs — save your work first.
- Options **1** and **4** briefly drop the network while the adapter
  cycles.
- Option **1** shows all eligible adapters with their current MAC and
  asks which one to change.

---

## Project layout

```
spoofer/
├── README.md
├── spoofer.sln
├── spoofer/
│   ├── spoofer.vcxproj
│   ├── spoofer.vcxproj.filters
│   ├── include/
│   │   ├── cleaner.h       cookie-clean interface + dynamic sqlite bindings
│   │   ├── console.h       ANSI colour macros, terminal helpers
│   │   ├── includes.h      umbrella include for the whole project
│   │   ├── logs.h          log level enum + write() entry point
│   │   ├── network.h       adapter listing / MAC read / MAC write
│   │   └── utils.h         elevation check, self-relaunch, env helper
│   └── src/
│       ├── cleaner.cpp
│       ├── console.cpp
│       ├── logs.cpp
│       ├── main.cpp        menu + dispatcher
│       ├── network.cpp
│       └── utils.cpp
```

---

## How it works

### MAC change

Enumerates
`HKLM\SYSTEM\CurrentControlSet\Control\Class\{4d36e972-e325-11ce-bfc1-08002be10318}\NNNN`,
filters out virtual / loopback / bluetooth / WAN miniport / TAP /
pseudo adapters, and lets you pick one. Writes a random
`02:XX:XX:XX:XX:XX` (locally-administered) to `NetworkAddress`, then
runs `netsh interface set interface … admin=disable` and `admin=enable`
so the driver picks up the new address.

### Cookie clear (Chromium)

Finds every `Cookies` file under each browser's `User Data` root, loads
sqlite dynamically (tries `winsqlite3.dll`, then `sqlite3.dll`, then
`mozsqlite3.dll`, then searches the browsers' installation directories),
and runs `DELETE FROM cookies WHERE host_key IN (...)`. Operates on a
`.tmp` copy and renames back afterwards so a locked WAL doesn't wreck
the original DB.

### Cookie clear (Firefox)

Same flow but against `moz_cookies` in each Firefox profile's
`cookies.sqlite`, with a fallback query if the `baseDomain` column
doesn't exist on older schemas.

---

## Troubleshooting

**"N profile(s) failed — close the browser and try again."**
Something is still holding the cookies DB open. Close every window of
that browser (including background instances in the tray) and re-run.

**"sqlite runtime not found."**
No `winsqlite3.dll`, `sqlite3.dll` or `mozsqlite3.dll` was found on
disk. Install any modern browser or update Windows.

**"failed to disable / enable adapter."**
`netsh` rejected the operation — usually because the adapter was
already down, or the friendly name has odd characters. Reboot and
re-run.

**UAC prompt cancelled.**
The app needs admin to write to `HKLM` and to run `netsh`. Accept the
UAC prompt when it appears.

---

## Disclaimer

This software is provided **as-is, with no warranty of any kind**,
express or implied. It is published for educational and reference
purposes only.

By downloading, building, or running this tool you agree that:

- You are solely responsible for how you use it.
- I am **not liable** for any damage, data loss, account bans,
  network disruption, hardware issues, legal consequences or any other
  problem — direct or indirect — that results from its use or misuse.
- You are responsible for complying with the terms of service of any
  platform, service, application or operating system you interact with.
- Modifying registry values, changing network adapter settings, killing
  processes, and deleting files can affect other software on your
  system. Only run this if you understand what it does.

If you don't agree with the above — don't use it.

---

## License

No license granted — this is published for reference. Fork freely; use
at your own risk.
