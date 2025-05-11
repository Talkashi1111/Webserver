#!/usr/bin/env python3
import cgi
import cgitb
import os
import pathlib
import html
import sys
import traceback

env_debug = os.environ.get("DEBUG", "")

# Enable CGI traceback for development
if env_debug == "1":
	cgitb.enable()

	# Print environment variables to stderr for debugging
	sys.stderr.write("===== ENVIRONMENT VARIABLES =====\n")
	for key, value in os.environ.items():
		sys.stderr.write(f"{key}: {value}\n")
	sys.stderr.write("================================\n")


def fail(msg, status="400 Bad Request"):
	"""
	Display error message to the user and exit the script.

	Args:
		msg (str): Error message to display
		status (str): HTTP status code and message
	"""
	sys.stdout.write(f"Status: {status}\r\n")
	sys.stdout.write("Content-Type: text/html; charset=UTF-8\r\n\r\n")
	sys.stdout.write(f"""
	<!DOCTYPE html>
	<html>
	<head>
		<title>Upload Error</title>
		<meta charset="UTF-8">
		<link rel="stylesheet" href="/style/styles_tal.css">
		<link href="https://fonts.googleapis.com/css2?family=Eczar:wght@400;800&family=Annie+Use+Your+Telescope&display=swap" rel="stylesheet">
	</head>
	<body>
		<nav>
			<ul>
				<li><a href="../index.html">Home</a></li>
				<li><a href="../pages/about.html">About</a></li>
			</ul>
		</nav>
		<main>
			<div class="wrapper">
				<h1 class="subtitle">Upload Failed</h1>
				<div class="line"></div>
				<p>{html.escape(msg)}</p>
				<a href="/pages/upload_demo.html" class="button">Try Again</a>
			</div>
		</main>
	</body>
	</html>
	""")
	sys.exit()


try:
	# CGI modules parse the multipart body for us
	form = cgi.FieldStorage()

	# Check if 'file' field exists in form
	fileitem = None
	if 'file' in form:
		fileitem = form['file']

	# Constants and configuration
	MAX_SIZE = 10 * 1024 * 1024  # 10 MB hard-limit

	# Get upload directory from environment variable or use default
	env_upload_dir = os.environ.get("UPLOAD_DIR", "")

	# Check for empty or quoted empty strings
	if env_upload_dir == "" or env_upload_dir == '""' or env_upload_dir == "''":
		sys.stderr.write("UPLOAD_DIR environment variable is empty or contains only quotes\n")
		fail("Server configuration error: Upload directory not properly configured",
			 status="500 Internal Server Error")

	sys.stderr.write(f"Using upload directory: {env_upload_dir}\n")

	# Basic validation
	if fileitem is None or not hasattr(fileitem, 'filename') or not fileitem.filename:
		fail("No file received")

	# Size check (FieldStorage already stored the file in a temp file)
	fileitem.file.seek(0, os.SEEK_END)
	size = fileitem.file.tell()
	fileitem.file.seek(0)
	if size > MAX_SIZE:
		fail("File too large (limit 10 MB)")

	# Sanitize filename
	safe_name = os.path.basename(fileitem.filename)
	safe_name = safe_name.replace(" ", "_")

	# Prepare destination path
	dest_path = pathlib.Path(env_upload_dir) / safe_name
	dest_path.parent.mkdir(parents=True, exist_ok=True)

	# Write the file
	with open(dest_path, "wb") as f:
		f.write(fileitem.file.read())

	# Success response with proper UTF-8 encoding and CSS
	sys.stdout.write("Content-Type: text/html; charset=UTF-8\r\n\r\n")
	sys.stdout.write(f"""
	<!DOCTYPE html>
	<html>
	<head>
		<title>Upload Success</title>
		<meta charset="UTF-8">
		<link rel="stylesheet" href="/style/styles_tal.css">
		<link href="https://fonts.googleapis.com/css2?family=Eczar:wght@400;800&family=Annie+Use+Your+Telescope&display=swap" rel="stylesheet">
	</head>
	<body>
		<nav>
			<ul>
				<li><a href="../index.html">Home</a></li>
				<li><a href="../pages/about.html">About</a></li>
			</ul>
		</nav>
		<main>
			<div class="wrapper">
				<h1 class="annie-font">Upload successful 🎉</h1>
				<div class="line"></div>
				<p>Saved as: <code>{html.escape(dest_path.name)}</code></p>
				<a href="/pages/upload_demo.html" class="button">Upload Another File</a>
			</div>
		</main>
	</body>
	</html>
	""")

except Exception as e:
	# Return a user-friendly error page
	sys.stdout.write("Status: 500 Internal Server Error\r\n")
	sys.stdout.write("Content-Type: text/html; charset=UTF-8\r\n\r\n")
	sys.stdout.write(f"""
	<!DOCTYPE html>
	<html>
	<head>
		<title>Server Error</title>
		<meta charset="UTF-8">
		<link rel="stylesheet" href="/style/styles_tal.css">
		<link href="https://fonts.googleapis.com/css2?family=Eczar:wght@400;800&family=Annie+Use+Your+Telescope&display=swap" rel="stylesheet">
	</head>
	<body>
		<nav>
			<ul>
				<li><a href="../index.html">Home</a></li>
				<li><a href="../pages/about.html">About</a></li>
			</ul>
		</nav>
		<main>
			<div class="wrapper">
				<h1 class="subtitle">Upload Failed</h1>
				<div class="line"></div>
				<p>An error occurred while processing your upload.</p>
				<a href="/pages/upload_demo.html" class="button">Try Again</a>
			</div>
		</main>
	</body>
	</html>
	""")
	# Log the error - this won't be shown to users but will be in server logs
	sys.stderr.write(f"Upload.py error: {str(e)}\n")
	traceback.print_exc(file=sys.stderr)
	#TODO: add a toolbar to the error page
