#!/usr/bin/env python3

import os
import urllib.parse

UPLOAD_DIR = '../uploads'  # Directory where files are stored

# Print the HTTP header
print("Content-Type: text/html\n")

# List available files
print("<h1>Available Files for Download</h1><ul>")

for filename in os.listdir(UPLOAD_DIR):
    filepath = os.path.join(UPLOAD_DIR, filename)
    if os.path.isfile(filepath):
        encoded_name = urllib.parse.quote(filename)  # Encode filename for URL safety
        print(f'<li><a href="/uploads/{encoded_name}">{filename}</a></li>')

print("</ul>")
