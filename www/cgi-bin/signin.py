#!/usr/bin/env python3
import cgi
import os
import sys

sys.stdout.write("Content-type: text/html\r\n\r\n")  # Ensures correct header format with CRLF

form = cgi.FieldStorage()
username = form.getvalue("username")

if username:
    print(f"""
    <html>
    <head>
        <title>Welcome</title>
    </head>
    <body>
        <h1>Welcome, {username}!</h1>
        <p>You have successfully signed in.</p>
        <a href="/">Back to Home</a>
    </body>
    </html>
    """)
else:
    print("""
    <html>
    <head>
        <title>Sign In</title>
    </head>
    <body>
        <form action="/cgi-bin/signin.py" method="post">
            <label for="username">Username:</label>
            <input type="text" name="username" id="username" required>
            <button type="submit">Sign In</button>
        </form>
    </body>
    </html>
    """)
