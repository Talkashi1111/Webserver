#!/usr/bin/env python3
import cgi
import cgitb
import os
import sys
import random
import string
import time
import http.cookies
import urllib.parse

# Enable debugging if requested
env_debug = os.environ.get("DEBUG", "")
if env_debug == "1":
	cgitb.enable()

	sys.stderr.write("===== ENVIRONMENT VARIABLES =====\n")
	for key, value in os.environ.items():
		sys.stderr.write(f"{key}: {value}\n")
	sys.stderr.write("================================\n")

def generate_session_id(length=32):
	"""Generate a random session ID"""
	chars = string.ascii_letters + string.digits
	return ''.join(random.choice(chars) for _ in range(length))

def get_current_session():
	"""Check if user has a valid session"""
	cookie = http.cookies.SimpleCookie(os.environ.get("HTTP_COOKIE", ""))
	if "session_id" in cookie:
		session_id = cookie["session_id"].value
		session_file = f"/tmp/sess_{session_id}"
		if os.path.exists(session_file):
			try:
				with open(session_file, "r") as f:
					username = f.read().strip()
					return username, session_id, None
			except Exception as e:
				return None, session_id, f"Error reading session file: {str(e)}"
		else:
			# Session file doesn't exist but we have a cookie
			return None, session_id, "Session expired or not found"
	return None, None, None

# Check if it's a logout request
query_string = os.environ.get("QUERY_STRING", "")
params = urllib.parse.parse_qs(query_string)
logout = "logout" in params

# Check current session
username, session_id, session_error = get_current_session()

# Handle logout - even with an invalid session, we should clear the cookie
if logout and session_id:
    # Try to remove session file if it exists
    session_file = f"/tmp/sess_{session_id}"
    try:
        if os.path.exists(session_file):
            os.remove(session_file)
    except:
        pass

    # Set expired cookie to remove it
    sys.stdout.write("Content-type: text/html\r\n")
    sys.stdout.write("Set-Cookie: session_id=deleted; Path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.write("""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <title>Logged Out</title>
        <link rel="stylesheet" href="../style/styles_tal.css">
        <meta http-equiv="refresh" content="2;url=/pages/login.html">
        <style>
            .logout-container {
                max-width: 600px;
                margin: 50px auto;
                padding: 20px;
                background: #f9f9f9;
                border-radius: 8px;
                box-shadow: 0 0 10px rgba(0,0,0,0.1);
                text-align: center;
            }
        </style>
    </head>
    <body>
        <div class="logout-container">
            <h1>Logged Out Successfully</h1>
            <p>You have been logged out. Redirecting to login page...</p>
            <a href="/pages/login.html" class="button">Back to Login</a>
        </div>
    </body>
    </html>
    """)
    sys.exit(0)

# Handle invalid session (cookie exists but session file doesn't)
if session_id and session_error:
    # Invalid session, clear the cookie and show login page with error message
    sys.stdout.write("Content-type: text/html\r\n")
    sys.stdout.write("Set-Cookie: session_id=deleted; Path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.write(f"""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <title>Session Error</title>
        <link rel="stylesheet" href="../style/styles_tal.css">
        <style>
            .error-container {{
                max-width: 600px;
                margin: 50px auto;
                padding: 20px;
                background: #fff0f0;
                border-radius: 8px;
                box-shadow: 0 0 10px rgba(0,0,0,0.1);
                text-align: center;
            }}
            .error-message {{
                color: #e53935;
                font-weight: bold;
            }}
        </style>
    </head>
    <body>
        <nav>
            <ul>
                <li><a href="../index.html">Home</a></li>
                <li><a href="/pages/about.html">About</a></li>
            </ul>
        </nav>
        <main>
            <div class="error-container">
                <h1>Session Error</h1>
                <p class="error-message">{session_error}</p>
                <p>Your session has expired or was not found. Please log in again.</p>
                <a href="/pages/login.html" class="button">Log In</a>
            </div>
        </main>
        <footer>
            <p>© 2025 Web Server Project. All rights reserved.</p>
        </footer>
    </body>
    </html>
    """)
    sys.exit(0)

# If user is already logged in, show welcome page
if username and session_id and not logout:
	# User is already logged in, show welcome page
	sys.stdout.write("Content-type: text/html\r\n")
	sys.stdout.write("\r\n")  # Additional \r\n to separate headers and body
	sys.stdout.write(f"""
	<!DOCTYPE html>
	<html lang="en">
	<head>
		<meta charset="UTF-8">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		<title>Welcome {username}</title>
		<link rel="stylesheet" href="../style/styles_tal.css">
		<style>
			.welcome-container {{
				max-width: 600px;
				margin: 50px auto;
				padding: 20px;
				background: #f9f9f9;
				border-radius: 8px;
				box-shadow: 0 0 10px rgba(0,0,0,0.1);
				text-align: center;
			}}
			.welcome-container h2 {{
				color: #4caf50;
			}}
			.session-info {{
				margin-top: 20px;
				padding: 10px;
				background: #e8f5e9;
				border-radius: 4px;
				font-family: monospace;
				text-align: left;
			}}
			.button {{
				display: inline-block;
				padding: 10px 20px;
				margin-top: 20px;
				background: #4caf50;
				color: white;
				text-decoration: none;
				border-radius: 4px;
			}}
		</style>
	</head>
	<body>
		<nav>
			<ul>
				<li><a href="../index.html">Home</a></li>
				<li><a href="/pages/about.html">About</a></li>
			</ul>
		</nav>
		<main>
			<div class="welcome-container">
				<h1>Welcome!</h1>
				<h2>Hello, {username}!</h2>
				<p>You are currently logged in.</p>

				<div class="session-info">
					<p><strong>Session ID:</strong> {session_id}</p>
					<p><strong>Session File:</strong> /tmp/sess_{session_id}</p>
				</div>

				<a href="/cgi-bin/login.py?logout=1" class="button">Logout</a>
				<a href="/" class="button">Home</a>
			</div>
		</main>
		<footer>
			<p>© 2025 Web Server Project. All rights reserved.</p>
		</footer>
	</body>
	</html>
	""")
	sys.exit(0)

# Parse form data with error handling
try:
	form = cgi.FieldStorage()
	username = form.getvalue("username")
except Exception as e:
	# Generate error response
	sys.stdout.write("Content-type: text/html\r\n")
	sys.stdout.write("\r\n")  # Additional \r\n to separate headers and body
	sys.stdout.write(f"""
	<!DOCTYPE html>
	<html lang="en">
	<head>
		<title>Error</title>
		<link rel="stylesheet" href="../style/styles_tal.css">
	</head>
	<body>
		<div class="welcome-container">
			<h1>Error</h1>
			<p>An error occurred while processing your request: {str(e)}</p>
			<a href="/pages/login.html" class="button">Back to Login</a>
		</div>
	</body>
	</html>
	""")
	sys.exit(1)

# Generate response with proper headers
sys.stdout.write("Content-type: text/html\r\n")

if username:
	try:
		# Generate a session ID
		session_id = generate_session_id()

		# Store username in session file
		session_file = f"/tmp/sess_{session_id}"
		try:
			with open(session_file, "w") as f:
				f.write(username)
		except IOError as e:
			raise Exception(f"Could not write session file: {str(e)}")

		# Set cookie that expires in 1 hour
		sys.stdout.write(f"Set-Cookie: session_id={session_id}; Path=/; Max-Age=3600\r\n")
		sys.stdout.write("\r\n")  # Additional \r\n to separate headers and body

		# Generate welcome page with session info
		sys.stdout.write(f"""
		<!DOCTYPE html>
		<html lang="en">
		<head>
			<meta charset="UTF-8">
			<meta name="viewport" content="width=device-width, initial-scale=1.0">
			<title>Welcome {username}</title>
			<link rel="stylesheet" href="../style/styles_tal.css">
			<style>
				.welcome-container {{
					max-width: 600px;
					margin: 50px auto;
					padding: 20px;
					background: #f9f9f9;
					border-radius: 8px;
					box-shadow: 0 0 10px rgba(0,0,0,0.1);
					text-align: center;
				}}
				.welcome-container h2 {{
					color: #4caf50;
				}}
				.session-info {{
					margin-top: 20px;
					padding: 10px;
					background: #e8f5e9;
					border-radius: 4px;
					font-family: monospace;
					text-align: left;
				}}
				.button {{
					display: inline-block;
					padding: 10px 20px;
					margin-top: 20px;
					background: #4caf50;
					color: white;
					text-decoration: none;
					border-radius: 4px;
				}}
			</style>
		</head>
		<body>
			<nav>
				<ul>
					<li><a href="../index.html">Home</a></li>
					<li><a href="/pages/about.html">About</a></li>
				</ul>
			</nav>
			<main>
				<div class="welcome-container">
					<h1>Welcome!</h1>
					<h2>Hello, {username}!</h2>
					<p>You have successfully logged in.</p>

					<div class="session-info">
						<p><strong>Session ID:</strong> {session_id}</p>
						<p><strong>Session File:</strong> /tmp/sess_{session_id}</p>
						<p><strong>Cookie Expiration:</strong> 1 hour</p>
					</div>

					<a href="/cgi-bin/login.py?logout=1" class="button">Logout</a>
					<a href="/" class="button">Home</a>
				</div>
			</main>
			<footer>
				<p>© 2025 Web Server Project. All rights reserved.</p>
			</footer>
		</body>
		</html>
		""")
	except Exception as e:
		# If something went wrong, show error page
		sys.stdout.write("\r\n")  # Additional \r\n to separate headers and body
		sys.stdout.write(f"""
		<!DOCTYPE html>
		<html lang="en">
		<head>
			<meta charset="UTF-8">
			<title>Login Error</title>
			<link rel="stylesheet" href="../style/styles_tal.css">
			<style>
				.error-container {{
					max-width: 600px;
					margin: 50px auto;
					padding: 20px;
					background: #fff0f0;
					border-radius: 8px;
					box-shadow: 0 0 10px rgba(0,0,0,0.1);
					text-align: center;
				}}
				.error-message {{
					color: #e53935;
					font-weight: bold;
				}}
			</style>
		</head>
		<body>
			<div class="error-container">
				<h1>Login Error</h1>
				<p class="error-message">An error occurred during login: {str(e)}</p>
				<a href="/pages/login.html" class="button">Try Again</a>
			</div>
		</body>
		</html>
		""")
else:
	# If no username was submitted, redirect back to login page
	sys.stdout.write("Location: /pages/login.html\r\n")
	sys.stdout.write("\r\n")  # Additional \r\n to separate headers and body
