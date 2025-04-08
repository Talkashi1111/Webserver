#!/usr/bin/env python3

import cgi
import os

UPLOAD_DIR = "../uploads"
os.makedirs(UPLOAD_DIR, exist_ok=True)

print("Content-Type: text/html\n")

form = cgi.FieldStorage()

if "file" not in form:
    print("<h1>No file uploaded</h1>")
else:
    uploaded_file = form["file"]
    if uploaded_file.filename:
        filename = os.path.basename(uploaded_file.filename)
        filepath = os.path.join(UPLOAD_DIR, filename)
        with open(filepath, "wb") as f:
            f.write(uploaded_file.file.read())
        print(f"<h1>File '{filename}' uploaded successfully.</h1>")
    else:
        print("<h1>No file selected</h1>")
