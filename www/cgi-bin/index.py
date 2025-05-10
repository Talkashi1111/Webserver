#!/usr/bin/env python3
import os

# Get server name and port from environment
server_name = os.environ.get('SERVER_NAME', 'unknown')
server_port = os.environ.get('SERVER_PORT', 'unknown')

# Print the HTTP headers
print ("Content-type:text/html\r\n\r\n")
print()
print(f"""
<html>
<head>
    <title>CGI Server Info</title>
</head>
<body>
    <h1>Server Information</h1>
    <p>Server Name: {server_name}</p>
    <p>Server Port: {server_port}</p>
</body>
</html>
""")
