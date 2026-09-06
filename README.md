# NeuraEase — Web tDCS Controller

**NeuraEase** is a browser-based interface designed to communicate with DIY neurostimulation hardware via the **Web Serial API**. 

> ⚠️ **Disclaimer:** This is an experimental test-protocol project built strictly for research, simulation, and testing purposes. It is not an approved medical device.

---

### 📌 Project Overview & Purpose
The platform tests and tracks stimulation protocols across three experimental focus areas:
* **Anxiety / Depression:** Left DLPFC (F3) anode target protocol.
* **Cognitive Enhancement:** Frontal cortex montage targeting focus.
* **Pain Management:** Motor cortex (C3) montage targeting pain modulation.

It allows operators to configure session parameters (duration and safety-capped current), track real-time hardware status, and record pre- and post-session mood shifts to observe outcome trends over time.

---

### 🛠️ Hardware Requirements
To complete the full physical setup, you will need:
* **Arduino Uno** (or compatible microcontroller)
* **tDCS Current Control Circuit** (constant-current source / regulator stage)
* **tDCS Electrodes & Sponges** (with saline solution)
* **Connecting leads / jumper wires**
* **USB Cable** (for serial communication with the browser)
* **Chromium-based browser** (Chrome, Edge, or Opera with Web Serial API enabled)

---

### ✨ Key Features
* **Browser-to-Hardware Control:** Direct two-way serial communication with the Arduino without requiring native desktop apps.
* **Preset Montage Guides:** Visual anode/cathode placement instructions for each testing protocol.
* **Safety Limits:** Enforced default parameter caps (≤3.0 mA, ≤5 min) for initial trials.
* **Outcome Tracking:** Pre/post session mood check-ins, progression visualization, and CSV/PNG data export.
