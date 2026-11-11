# Simulator reference assets

Source: user-provided simulator-main.zip, SHA-256 cab5aa612c80f0128323c0713d35d4213bf775c2f730306b3abcae903622d332.
Only the active Unity Build, its template files, FileSaver, Acorn and JS-Interpreter are included. No node_modules, database, telemetry server or Build-old is included.
Original JS copyright/license headers are preserved. JS-Interpreter identifies the Apache 2.0 license in its header. This notice does not relicense the Unity build or other reference assets.

UnityBuild/index.html is a compatibility copy: automatic legacy polling/storage startup is disabled, innerHTML accesses are replaced with textContent, and the controlled runtime scripts are added. The Unity binaries are unchanged.
runtime-core.js and runtime.js are IDE integration code. They expose only the documented prototype subset; the reference's entire API is not certified.


simulation_font.rs uses printable ASCII glyph bytes from app/public/js/simulator/workers/ht16k33.js in the same ZIP. The source worker was compared byte-for-byte with the extracted reference. The checked device-behavior generator lowers text/scroll to ht16k33.show bitmaps and delays, retaining the original x-major/bit-zero LED mapping. Unity binaries remain unchanged. No additional license for the original font or Unity assets is implied.

Local buzzer note frequencies and duration mapping were checked against app/public/js/simulator/kidbright32/sound.js and the sound wrappers in simulator.js from the same reference ZIP. The IDE renderer uses a monophonic square wave and does not change Unity binaries. sound.tone is a documented local Hz extension, not a claimed Unity source-execution endpoint.
