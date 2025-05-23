# Webserv Configuration Documentation

This document explains the structure and directives of the webserver configuration file. The configuration is inspired by NGINX.

---

## Global Defaults (If Not Overridden)

These defaults apply if a directive is not explicitly specified in a server block:

- **Listen**
  - **Context:** Server block
  - **Default:** `0.0.0.0:80`
  - **Occurrence:** If no `listen` directive is provided, the server defaults to listening on all interfaces (0.0.0.0) at port 80.

- **Server Name**
  - **Context:** Server block
  - **Default:** Empty (no name)
  - **Occurrence:** May be specified multiple times to add aliases.

- **Root**
  - **Context:** Server (or Location) block
  - **Default:** Calculates current working directory + `/www`, fallback to `/var/www/html`.
  - **Purpose:** Sets the base directory for mapping request URIs to the filesystem.

- **Index**
  - **Context:** Server (or Location) block
  - **Default:** `index.html`
  - **Purpose:** Specifies the default file to serve when a directory is requested.

- **Client Timeout**
  - **Context:** Global block
  - **Default:** `75`
  - **Purpose:** Maximum time in seconds to wait for client activity before timing out.

- **Client Header Buffer Size**
  - **Context:** Global block
  - **Default:** `2k`
  - **Purpose:** Limits the size of the header buffer for client requests.

- **Client Max Body Size**
  - **Context:** Global block
  - **Default:** `1m`
  - **Purpose:** Maximum allowed size for the client’s request body.

- **Error Pages**
  - **Context:** Server block
  - **Default:**
    - For `404`: `/404.html`
    - For `500, 502, 503, 504`: `/50x.html`
  - **Purpose:** Custom pages to serve when errors occur.

- **Allowed Methods**
  - **Context:** Server (or Location) block
  - **Default:** `GET`
  - **Purpose:** Specifies which HTTP methods are permitted.

- **Autoindex**
  - **Context:** Server (or Location) block
  - **Default:** `off`
  - **Purpose:** Enables or disables automatic directory listing when no index file is found.

---

## Global Directives

These directives must be specified in the global scope (outside of any server block):

- **Client Timeout**
  - **Context:** Global only
  - **Default:** `75`
  - **Usage:** `client_timeout 75;`
  - **Purpose:** Maximum time in seconds to wait for client activity before timing out.

- **Client Header Buffer Size**
  - **Context:** Global only
  - **Default:** `2k`
  - **Usage:** `client_header_buffer_size <size>;`
  - **Example:** `client_header_buffer_size 2k;`
  - **Purpose:** Sets the size of the buffer used for reading client request headers.
  - **Format:** Size can be specified in:
    - Bytes (no suffix): `1024`
    - Kilobytes: `1k` or `1K`
    - Megabytes: `1m` or `1M`
  - **Occurrence:** Once per configuration file
  - **Note:** If a client sends headers larger than this size, the server will
    return a 413 Request Entity Too Large error.

- **Client Max Body Size**
	- **Context:** Global only
	- **Default:** `1m`
	- **Usage:** `client_max_body_size <size>;`
	- **Example:** `client_max_body_size 2m;`
	- **Purpose:** Limits the size of client request bodies. If the Content-Length header exceeds this value, returns 413 (Request Entity Too Large).
	- **Format:** Size can be specified in:
		- Bytes (no suffix): `1024`
		- Kilobytes: `1k` or `1K`
		- Megabytes: `1m` or `1M`
	- **Occurrence:** Once per configuration file


## Server Block Directives

Within a `server` block, you can specify:

- **listen**
  - **Usage:** `listen <IP/hostname>:<port>;`
  - **Occurrence:** May be specified multiple times (e.g., to bind to different addresses/ports).

- **server_name**
  - **Usage:** `server_name name1 name2 ...;`
  - **Occurrence:** May be declared multiple times; all declared names are considered.

- **root**
  - **Usage:** `root /path/to/directory;`
  - **Purpose:** Sets the base directory for all files served by the server (unless overridden in a location).

- **index**
  - **Usage:** `index file1 file2 ...;`
  - **Purpose:** Lists default files to serve for directory requests.

- **error_page**
  - **Usage:** `error_page <code> [<code> ...] /path/to/error/page;`
  - **Occurrence:** Can be defined for multiple error codes.

- **allowed_methods**
  - **Usage:** `allowed_methods GET POST DELETE;`
  - **Purpose:** Sets the allowed HTTP methods.

- **autoindex**
  - **Usage:** `autoindex on;` or `autoindex off;`

- **CGI Configuration (Custom Directives)**
  - **Usage:**
    ```nginx
    cgi_bin .pl  /usr/bin/perl;
    cgi_bin .py  /usr/bin/python3;
    ```
  - **Purpose:** Maps file extensions to the CGI executable that will process them. The server will build the `PATH_INFO` by combining the `root` value and the URI from the request.

- **return**
  - **Usage:** `return <status> <URL or "text">;`
  - **Purpose:** Issues an HTTP redirect or returns a specific response.
  - **Examples:**
    ```nginx
    # URL redirects
    return 301 http://example.com;          # Permanent redirect
    return 302 https://example.com/page;    # Temporary redirect

    # Text responses (must be in double quotes)
    return 200 "Hello World";               # Success response
    return 403 "Access Denied";             # Forbidden response
    ```
  - **Valid Status Codes:**
    - `301`, `302`, `303`, `307`, `308` for redirects
    - `100`-`599` for text responses
  - **Format Rules:**
    - URLs must start with `http://` or `https://`
    - Text content must be enclosed in double quotes
  - **Precedence:**
    - If defined in server context, it takes precedence over all location blocks
    - Only the first return directive in a context is effective
    - Example:
      ```nginx
      server {
          return 301 http://example.com;     # This wins, immediate return

          location / {
              return 302 http://other.com;   # Never reached
          }
      }

      server {
          location / {
              return 301 http://first.com;   # This wins
              return 302 http://second.com;  # Ignored
          }
      }
      ```

---

## Location Block Directives

Location blocks are nested inside server blocks and apply to specific URI patterns. They can override server-level settings.

- **location**
  - **Usage:** `location /path/ { ... }`
  - **Occurrence:** Multiple location blocks can be declared, with more specific ones (e.g., `/foo/bar/`) taking precedence over less specific ones (e.g., `/foo/`).

- **root** (Override)
  - **Usage:** May be redefined in a location block to change the mapping for that URI.

- **index** (Override)
  - **Usage:** May be redefined for a particular location.

- **allowed_methods** (Override)
  - **Usage:** May be redefined to restrict or expand allowed methods for that location.

- **autoindex** (Override)
  - **Usage:** Enables or disables directory listing for that location.

- **return**
  - **Usage:** Provides a response for that specific location; server-level return directive takes precedence over location-level return directive.

- **upload_directory**
  - **Usage:** Used within an upload-specific location to specify where uploaded files should be stored.
  - **Example:** `upload_directory /path/to/uploads;`
  - **Configuration Requirements:**
    - Must be configured in a location block that is a prefix of the upload.py path
    - For example, if upload.py is in the /cgi-bin directory, the location block should be:
      ```nginx
      location /cgi-bin {
          upload_directory /home/csa/Documents/taltol/webserver/www/uploads/;
      }
      ```
    - This configuration will be passed as an environment variable to the upload.py CGI script
  - **Note:** If not configured correctly, file uploads may fail with a configuration error

---

## Example Configuration Overview

Below is a summary of an example configuration:

```nginx
# Global Defaults (if not explicitly overridden):
# - Listens on 0.0.0.0:80
# - server_name is empty
# - root is current working directory + '/www'
# - index is index.html
# - client_timeout is 75
# - client_header_buffer_size is 2k
# - client_max_body_size is 1m
# - Default error pages:
#    404             -> /404.html
#    500 502 503 504 -> /50x.html
# - Allowed methods: GET
# - Autoindex: off

server {
    # Empty server block - can serve as a fallback
}

client_timeout 75;
client_header_buffer_size 2k;
client_max_body_size 1m;

server {
    listen 80;
    listen 0.0.0.0:81;
    listen localhost:90;

    server_name example.org www.example.org example.org;
    server_name example.org www.example.com;

    root /var/www/html;

    # CGI Configuration for .pl and .py files:
    cgi_bin .pl  /usr/bin/perl;
    cgi_bin .py  /usr/bin/python3;

    error_page 404 /404.html;    # NOT FOUND
    error_page 400 /400.html;    # BAD REQUEST
    error_page 405 /405.html;    # METHOD NOT ALLOWED
    error_page 414 /414.html;    # URI MAX
    error_page 501 /501.html;    # NOT IMPLEMENTED
    error_page 500 /500.html;    # Internal Server Error
    error_page 502 503 504 /50x.html;

    autoindex on;
    allowed_methods GET POST DELETE;

    # Default server-level redirect for unmatched requests:
    return 301 http://example.com/default;


    index index.html;

    # Location blocks:

    # Default location for static content:
    location / {
        index index.html;
    }

    location /foo/ {
        allowed_methods GET;
    }

    location /foo/bar/ {
        autoindex off;
    }

    # A location block overriding the server-level return:
    location /special {
        return 302 http://example.com/special;
    }

    # Location block for file uploads:
    location /upload {
        root /var;
        allowed_methods POST;
        upload_directory /var/www/uploads;
    }
}

server {
    listen localhost;
    server_name static.site;
    index index.html home.html;
    error_page 404 /404.html;

    # Default location for static files:
    location / {
        autoindex off;
    }

    # HTTP Redirect example:
    location /old-path {
        return 301 http://example.com/new-path;
    }
}
```
