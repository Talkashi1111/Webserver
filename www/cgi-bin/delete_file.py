#!/usr/bin/env python3
import os
import sys
import html
import urllib.parse

env_debug = os.environ.get("DEBUG", "")
if env_debug == "1":
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
	<title>Delete Error</title>
	<meta charset="UTF-8">
	<link rel="stylesheet" href="/style/styles_tal.css">
	<link href="https://fonts.googleapis.com/css2?family=Eczar:wght@400;800&family=Annie+Use+Your+Telescope&display=swap" rel="stylesheet">
</head>
<body>
	<main>
		<div class="wrapper">
			<h1 class="subtitle">Delete Failed</h1>
			<div class="line"></div>
			<p>{html.escape(msg)}</p>
			<a href="/pages/delete_demo.html" class="button">Go Back</a>
		</div>
	</main>
</body>
</html>
""")
	sys.exit()

try:
	# Check if the request method is DELETE
	request_method = os.environ.get("REQUEST_METHOD", "")

	if request_method != "DELETE":
		fail(f"This script only accepts DELETE requests, got: {request_method}",
			 status="405 Method Not Allowed")

	# Parse the query string from QUERY_STRING environment variable
	query_string = os.environ.get("QUERY_STRING", "")
	if not query_string:
		fail("No query parameters provided")

	# Parse the query string to get the filename
	query_params = urllib.parse.parse_qs(query_string)

	if "filename" not in query_params:
		fail("No filename specified for deletion")

	filename = query_params["filename"][0]  # Get the first value for the filename parameter

	# Get upload directory from environment variable or use default
	upload_dir = os.environ.get("UPLOAD_DIR", "")

	# Check for empty or quoted empty strings
	if upload_dir == "" or upload_dir == '""' or upload_dir == "''":
		sys.stderr.write("UPLOAD_DIR environment variable is empty or contains only quotes\n")
		fail("Server configuration error: Upload directory not properly configured",
			 status="500 Internal Server Error")

	# Verify that the file exists and is within the upload directory
	file_path = os.path.join(upload_dir, filename)

	# Normalize paths to prevent directory traversal attacks
	upload_dir_real = os.path.realpath(upload_dir)
	file_path_real = os.path.realpath(file_path)

	# Check if the file path is a subpath of the upload directory
	if not file_path_real.startswith(upload_dir_real):
		fail("Access denied: Cannot delete files outside the upload directory",
			 status="403 Forbidden")

	# Check if the file exists
	if not os.path.isfile(file_path_real):
		fail(f"File not found: {html.escape(filename)}",
			 status="404 Not Found")

	# Delete the file
	try:
		os.unlink(file_path_real)

		# Return success response
		sys.stdout.write("Status: 200 OK\r\n")
		sys.stdout.write("Content-Type: text/html; charset=UTF-8\r\n\r\n")
		sys.stdout.write(f"""
<!DOCTYPE html>
<html>
<head>
	<title>File Deleted</title>
	<meta charset="UTF-8">
	<link rel="stylesheet" href="/style/styles_tal.css">
	<link href="https://fonts.googleapis.com/css2?family=Eczar:wght@400;800&family=Annie+Use+Your+Telescope&display=swap" rel="stylesheet">
</head>
<body>
	<main>
		<div class="wrapper">
			<h1 class="annie-font">File Deleted Successfully 🗑️</h1>
			<div class="line"></div>
			<p>The file <code>{html.escape(filename)}</code> has been deleted.</p>
			<a href="/pages/delete_demo.html" class="button">Go Back</a>
		</div>
	</main>
</body>
</html>
""")

	except Exception as e:
		fail(f"Error deleting file: {html.escape(str(e))}",
			 status="500 Internal Server Error")

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
	<main>
		<div class="wrapper">
			<h1 class="subtitle">Delete Failed</h1>
			<div class="line"></div>
			<p>An error occurred while processing your request.</p>
			<a href="/pages/delete_demo.html" class="button">Go Back</a>
		</div>
	</main>
</body>
</html>
""")
	# Log the error - this won't be shown to users but will be in server logs
	sys.stderr.write(f"delete_file.py error: {str(e)}\n")
	import traceback
	traceback.print_exc(file=sys.stderr)
