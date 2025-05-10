#!/usr/bin/env python3
"""
Robust CGI handler for POST requests.

 • Accepts url-encoded or JSON bodies.
 • Survives wrong or missing CONTENT_TYPE.
 • Parses data manually if cgi.FieldStorage() would block or crash.
"""

import os, sys, cgi, cgitb, html, json, urllib.parse
cgitb.enable()                         # comment out in production

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
<head><meta charset="utf-8"><title>Submission OK</title></head>
<body style="font-family: sans-serif">
  <h1>Form submission debug</h1>

  <h2>Parsed fields</h2>
  <ul>
    <li>username&nbsp;=&nbsp;{username}</li>
    <li>email&nbsp;&nbsp;&nbsp;&nbsp;=&nbsp;{emailaddress}</li>
  </ul>
""")

interesting = {k: v for k, v in os.environ.items()
               if k.startswith(("REQUEST_", "CONTENT_", "HTTP_"))}

env_table = "\n".join(
    f"<tr><th style=\"text-align:left\">{html.escape(k)}</th><td>{html.escape(v)}</td></tr>"
    for k, v in sorted(interesting.items())
)

print("""
<h2>CGI environment snapshot</h2>
<table style="border-collapse:collapse;font-family:monospace">
  <thead>
    <tr><th style="text-align:left;padding:4px 8px;border-bottom:2px solid #ccc">
        Variable</th>
        <th style="text-align:left;padding:4px 8px;border-bottom:2px solid #ccc">
        Value</th></tr>
  </thead>
  <tbody>
""" + env_table + """
  </tbody>
</table>
""")

print(f"""

  <p><a href="../pages/post_demo.html">Back to form</a></p>
</body>
</html>""")
