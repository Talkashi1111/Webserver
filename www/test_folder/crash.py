#!/usr/bin/env python3

# Print HTTP headers first so the server knows it's a CGI response
print("Content-type:text/html\r\n\r\n")
print("<html><head><title>Crash Test</title></head><body>")
print("<h1>This script will now crash...</h1>")

# Flush stdout to make sure the above gets sent
import sys
sys.stdout.flush()

# Different ways to crash the script
crash_method = 2  # Change this to use different crash methods

if crash_method == 1:
    # Division by zero error
    result = 1 / 0
elif crash_method == 2:
    # Raise an exception
    raise Exception("Intentionally crashing the script!")
elif crash_method == 3:
    # Reference a non-existent variable
    print(non_existent_variable)
elif crash_method == 4:
    # Segmentation fault (not always reliable)
    import ctypes
    ctypes.string_at(0)

# This code will never run
print("</body></html>")
