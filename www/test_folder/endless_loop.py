#!/usr/bin/env python3
import os
import time

# Print HTTP headers first
print("Content-type:text/html\r\n\r\n")
print("<html><head><title>Endless Loop</title></head><body>")
print("<h1>Starting endless loop...</h1>")
print("</body></html>")

# Flush stdout to ensure headers and initial content are sent
import sys
sys.stdout.flush()

# Start an endless loop
while True:
    pass
