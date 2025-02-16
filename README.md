
# Stress Testing Tool using wrk (for Server)

**wrk** is a modern HTTP tool capable of generating significant load when run on a single multi-core CPU. It uses multiple threads to work on several tasks at the same time, and it also utilizes smart systems (like **epoll** and **kqueue**) to quickly and efficiently detect events on many connections.

## Options

The basic usage of **wrk** is as follows:

```bash
wrk -t<number of threads> -c<number of connections> -d<duration in seconds>s ip_address:port
```

## Example

For instance, to run a test with 12 threads, establishing 10,000 connections, and running for 30 seconds, you would use:

```bash
wrk -t12 -c10000 -d30s http://localhost:8001/
```

Here:
- **-t12**: Runs 12 threads.
- **-c10000**: Establishes 10,000 connections.
- **-d30s**: Limits the test duration to 30 seconds.

## Note on Thread Limits

On the first run, if you try to run with more than 12 threads (e.g., `-t10000`), the system might not allow additional threads to run due to thread limits. To fix this, you can increase the limit using:

```bash
ulimit -n <number of threads wanted>
```

For example:

```bash
ulimit -n 100000
```

## Sample Output

A typical output might look like this:

```
Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    68.09ms  203.66ms   2.00s    91.74%
    Req/Sec   787.45      1.06k    8.14k    89.64%
  24318 requests in 3.23s, 3.69MB read
  Socket errors: connect 0, read 0, write 0, timeout 3
Requests/sec:   7522.12
Transfer/sec:      1.14MB
```

This output means that during the test:
- A total of **24,318 requests** were made.
- There were **no connection, read, or write errors** (only 3 timeouts were reported).
- The server achieved a throughput of approximately **7,522 requests per second** and transferred around **1.14MB per second**.
```


# Revised Listening Socket Management

## Handling Duplicate Bindings

### Issue
Multiple server configurations might specify the same port and IP address. For example, one configuration may request listening on `127.0.0.1:80` while another uses the wildcard `0.0.0.0:80`.

### Problem
Since `0.0.0.0` (the IPv4 wildcard) covers all interfaces (including `127.0.0.1`), binding to `0.0.0.0:80` after already binding to `127.0.0.1:80` causes the `bind()` call to fail with an "address already in use" error.

### Solution Implemented
When a request comes in to bind to a wildcard address (like `0.0.0.0`) on a port that is already in use by another server, the code will:
- **Close** the older socket file descriptor (fd).
- **Open** a new socket that listens on the wildcard.

This ensures that the new server can handle connections on that port for all IPs.

## IPv4 vs. IPv6 Wildcards

- **IPv4:** Uses `0.0.0.0` as the wildcard address.
- **IPv6:** Uses `::` as the wildcard address.

## Implementation Details
- A particular data structure is used to hold the port, hosts, and socket file descriptor:
	It is a map structured as: `<string port, <map<string host_ip, int socket_fd>>`.
- Two separate maps are maintained (one for IPv4 and one for IPv6) after calling `getaddrinfo()`.
- Each map uses the port number (an integer) as the key.
- The value for each key is another map that associates the IP address (as a `std::string`) to its socket fd.

## Final Structure
After processing, all the listener file descriptors (both IPv4 and IPv6) are merged into a single `std::set`, which is then passed to `epoll()` for event monitoring.

---

# Switching from poll() to epoll()

## Reason for the Change
- **Scalability and Performance:**
  For a webserver that listens on multiple server sockets (potentially many), `poll()` becomes inefficient because its performance degrades as the number of file descriptors increases.

## epoll Advantages
- `epoll()` is designed to handle a large number of file descriptors efficiently.
- It scales much better under heavy load compared to `poll()`.

## Result
- The webserver now uses `epoll()` for event handling.
- This change ensures efficient monitoring of all listener sockets, even as their number increases.
```
## TODO
1. keep track on new connection in order to close the fd of this connection after finishing. (right now there is a leak of open fd).
2. to pack it up in a class form that will be more CPP ( one class that will contain : epollfd, listeners, event_struct, and new client connection).
