# DNS Server

## Summary
•	Understood DNS packet (via Wireshark)\
•	Practiced the socket connection\
•	Created an analyzer for the incoming requests.\
•	Forwarded the client's inquiry to upper DNS server and replied with correct IP address\
•	Constructed caching feature and multi-threads functionality by pthread

## Task & Requirements

### 1.Standard Option
Accept a DNS “AAAA” query over TCP on port 8053. Forward it to a server whose IPv4 address is the first command-line argument and whose port is the second command-line argument. (For testing, use the value in /etc/resolv.conf on your server and port 53). Send the response back to the client who sent the request, over the same TCP connection. There will be a separate TCP connection for each query/response with the client. Log these events, as described below.
Note that DNS usually uses UDP, but this project will use TCP because it is a more useful skill for you to learn. A DNS message over TCP is slightly different from that over UDP: it has a two-byte header that specify the length (in bytes) of the message, not including the two-byte header itself [4, 5]. This means that you know the size of the message before you read it, and can malloc() enough space for it.\
\
Assume that there is only one question in the DNS request you receive, although the standard allows there to be more than one. If there is more than one answer in the reply, then only log the first one, but always reply to the client with the entire list of answers. If there is no answer in the reply, log the request line only. If the first answer in the response is not a AAAA field, then do not print a log entry (for any answer in the response).
The program should be ready to accept another query as soon as it has processed the previous query and response.

### 2.Cache Option
As above, but cache the five (5) most recent answers to queries. Answer directly if possible, instead of querying the server again.
Note that the ID field of the reply must be changed for each query so that the client doesn’t report an error. The time-to-live (TTL) field of the reply must also be decremented by the amount of time which has passed.
\
Note also that cache entries can expire. A cache entry should not be used once it has expired and a new query should be sent to the recursive server.\
\
If a new answer is received when the cache contains an expired entry, the expired entry (or another expired entry) should be overwritten. If there is no expired entry, any cache eviction policy is allowed.
\
Do not cache responses that do not contain answers (i.e., the address was not found). If multiple answers are returned, only cache the first one.
If you implementing caching, include the line “#define CACHE” in your code. Otherwise, this functionality will not be tested/marked.

### 3.Non-blocking Option
It can sometimes take a while for the server that was queried to give a response. To perform a recursive DNS lookup, many servers may need to be contacted (when starting from an empty cache, from a root DNS server down to an authoritative DNS server); any of them may be congested. Meanwhile, another request may have arrived, which the server cannot respond to if it was blocked by the completion of the first request.
This option extends both options above and mitigates this problem, by enabling the processing of new requests while waiting for prior requests to complete.
This may be done using multi-threading, or select(3)/epoll(7). Using multithreading may require explicit locking of shared resources, such as the cache, whereas a single threaded implementation using select() will not require that. However, using select() may require significant refactoring if it is not considered in the initial code design.

***
### Command Line
`make clean && make -B && ./dns_svr <server-ip> <server-port>`
  
### Example Output
#### _appears in log file_

***
_more details in project specification!_
