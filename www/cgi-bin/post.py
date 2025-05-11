#!/usr/bin/env python3
"""
Robust CGI handler for POST requests.

 • Accepts url-encoded or JSON bodies.
 • Survives wrong or missing CONTENT_TYPE.
 • Parses data manually if cgi.FieldStorage() would block or crash.
"""

import os, sys, cgi, cgitb, html, json, urllib.parse

# Enable CGI traceback for development
env_debug = os.environ.get("DEBUG", "")
if env_debug == "1":
	cgitb.enable()

	# Print environment variables to stderr for debugging
	sys.stderr.write("===== ENVIRONMENT VARIABLES =====\n")
	for key, value in os.environ.items():
		sys.stderr.write(f"{key}: {value}\n")
	sys.stderr.write("================================\n")

# --------------------------------------------------------------------- #
# helpers
# --------------------------------------------------------------------- #

def read_raw_body() -> bytes:
	"""Read CONTENT_LENGTH bytes from stdin (or b'' if none)."""
	length = int(os.getenv("CONTENT_LENGTH") or 0)
	return sys.stdin.buffer.read(length) if length else b""

def safe_url_decode(raw: bytes) -> dict[str, str]:
	"""Parse application/x-www-form-urlencoded bytes."""
	parsed = urllib.parse.parse_qs(raw.decode("utf-8", "replace"), keep_blank_values=True)
	# convert   {"k": ["v1", "v2"]} -> {"k": "v1"}  (first value only)
	return {k: v[0] for k, v in parsed.items()}

def form_data() -> dict[str, str]:
	"""
	Return a dict with the submitted fields, handling:
	  • correct CGI (normal case)
	  • JSON body
	  • url-encoded body but wrong CONTENT_TYPE
	"""
	ctype = (os.getenv("CONTENT_TYPE") or "").lower()
	# 1) try the canonical way first
	try:
		form = cgi.FieldStorage()          # may block / raise
		if form:                           # parsed OK
			return {k: form.getfirst(k, "") for k in form}
	except (TypeError, OSError):           # wrong mode / blocking
		pass                               # fall through to manual parsing

	raw = read_raw_body()

	# 2) JSON?
	if "application/json" in ctype and raw:
		try:
			data = json.loads(raw.decode("utf-8", "replace"))
			if isinstance(data, dict):
				return {str(k): str(v) for k, v in data.items()}
		except json.JSONDecodeError:
			pass                           # fall through

	# 3) assume url-encoded as last resort
	return safe_url_decode(raw)

def esc(s: str) -> str:
	return html.escape(s, quote=True)

# --------------------------------------------------------------------- #
# main
# --------------------------------------------------------------------- #

data = form_data()
username      = esc(data.get("username", ""))
emailaddress  = esc(data.get("emailaddress", ""))

# one and only one header block
print("Content-Type: text/html\r\n\r\n")

print(f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Submission OK</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Annie+Use+Your+Telescope&family=Eczar:wght@400..800&display=swap" rel="stylesheet">
  <link rel="stylesheet" href="../style/styles_tal.css">
  <style>
    .data-container {{
      background-color: rgba(32, 126, 7, 0.05);
      border-left: 4px solid #207e07;
      padding: 15px;
      margin: 20px 0;
      border-radius: 4px;
    }}
    .data-row {{
      display: flex;
      margin-bottom: 10px;
      align-items: center;
    }}
    .data-label {{
      font-weight: bold;
      width: 120px;
      color: #444;
      font-family: "Eczar", serif;
    }}
    .data-value {{
      flex-grow: 1;
      padding: 8px 12px;
      background-color: #fff;
      border-radius: 4px;
      box-shadow: 0 1px 3px rgba(0,0,0,0.1);
      transition: transform 0.3s, box-shadow 0.3s;
    }}
    .data-row:hover .data-value {{
      transform: translateY(-2px);
      box-shadow: 0 3px 8px rgba(32, 126, 7, 0.2);
    }}
  </style>
</head>
<body>
  <nav>
    <ul>
      <li><a href="../index.html">Home</a></li>
      <li><a href="../pages/about.html">About</a></li>
    </ul>
  </nav>
  <main>
    <h2 class="annie-font">Submission Successful</h2>
    <div class="wrapper">
      <h3 class="subtitle">Parsed fields</h3>
      <div class="line"></div>
      <div class="data-container">
        <div class="data-row">
          <div class="data-label">Username:</div>
          <div class="data-value">{username}</div>
        </div>
        <div class="data-row">
          <div class="data-label">Email:</div>
          <div class="data-value">{emailaddress}</div>
        </div>
      </div>
      <a href="../pages/post_demo.html" class="button">Back to form</a>
    </div>
  </main>
</body>
</html>""")

