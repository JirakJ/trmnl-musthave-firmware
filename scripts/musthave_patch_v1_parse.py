#!/usr/bin/env python3
"""One-off patch helper (protocol v1 parser + X-Frame-Id header). Idempotent; delete after commit."""
from pathlib import Path

root = Path(__file__).resolve().parent.parent

p = root / "lib/trmnl/include/api_types.h"
s = p.read_text(encoding="utf-8")
if "V1_ACTION_NONE" not in s:
    assert "struct ApiDisplayResponse {" in s
    s = s.replace("struct ApiDisplayResponse {", """// Protocol v1 (must-have fork): what the server wants the device to do with the panel.
enum V1Action { V1_ACTION_NONE = 0, V1_ACTION_PARTIAL = 1, V1_ACTION_FULL = 2 };
enum V1FullMode { V1_FULL_FULL = 0, V1_FULL_FAST = 1 };
enum V1SleepMode { V1_SLEEP_DEEP = 0, V1_SLEEP_LIGHT = 1 };

struct ApiDisplayResponse {""")
    old = "  String action;\n  String touchbar_mode;\n};"
    assert old in s
    s = s.replace(old, """  String action;
  String touchbar_mode;
  // protocol v1 (defaults keep stock servers working: full via image_url/filename)
  V1Action v1_action;
  String frame_id;
  String full_url;
  String regions_url;
  V1FullMode full_mode;
  V1SleepMode sleep_mode;
};""")
    old = "  bool imageCached;\n  int prevWakeTime;\n};"
    assert old in s
    s = s.replace(old, """  bool imageCached;
  int prevWakeTime;
  String frameId;  // protocol v1: frame currently on the panel (empty = unknown)
};""")
    p.write_text(s, encoding="utf-8")
    print("api_types.h patched")

p = root / "lib/trmnl/src/parse_response_api_display.cpp"
s = p.read_text(encoding="utf-8")
if "v1_action" not in s:
    old = '        .action = "",\n        .touchbar_mode = ""};\n  }'
    assert old in s
    s = s.replace(old, '''        .action = "",
        .touchbar_mode = "",
        .v1_action = V1_ACTION_FULL,
        .frame_id = "",
        .full_url = "",
        .regions_url = "",
        .full_mode = V1_FULL_FULL,
        .sleep_mode = V1_SLEEP_DEEP};
  }
  // protocol v1: missing/unknown values fall back to the stock behaviour (full refresh via image_url)
  String v1ActionStr = doc["action"] | "";
  V1Action v1Action = V1_ACTION_FULL;
  if (v1ActionStr == "none") v1Action = V1_ACTION_NONE;
  else if (v1ActionStr == "partial") v1Action = V1_ACTION_PARTIAL;
  String fullModeStr = doc["full_mode"] | "";
  String sleepModeStr = doc["sleep_mode"] | "";
  String imageUrl = doc["image_url"] | "";
  String fullUrl = doc["full_url"] | "";
  String frameId = doc["frame_id"] | "";
  String fileName = doc["filename"] | "";''')
    old = '      .action = doc["action"] | "",\n      .touchbar_mode = doc["touchbar_mode"] | ""};'
    assert old in s
    s = s.replace(old, '''      .action = doc["action"] | "",
      .touchbar_mode = doc["touchbar_mode"] | "",
      .v1_action = v1Action,
      .frame_id = frameId.length() ? frameId : fileName,
      .full_url = fullUrl.length() ? fullUrl : imageUrl,
      .regions_url = doc["regions_url"] | "",
      .full_mode = (fullModeStr == "fast") ? V1_FULL_FAST : V1_FULL_FULL,
      .sleep_mode = (sleepModeStr == "light") ? V1_SLEEP_LIGHT : V1_SLEEP_DEEP};''')
    p.write_text(s, encoding="utf-8")
    print("parser patched")

p = root / "lib/trmnl/src/api-client/request_headers.cpp"
s = p.read_text(encoding="utf-8")
if "X-Frame-Id" not in s:
    old = '  headers.push_back({"FW-Version", inputs.firmwareVersion});'
    assert old in s
    s = s.replace(old, old + '\n  if (inputs.frameId.length() > 0) headers.push_back({"X-Frame-Id", inputs.frameId});  // protocol v1')
    p.write_text(s, encoding="utf-8")
    print("headers patched")
print("done")
