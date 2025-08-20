# WebServer

📌 **Overview**

This project consists of creating a fully functional HTTP server from scratch, without using external libraries or frameworks. The goal is to understand and implement the core mechanisms that make the web work, from request handling to response delivery, while respecting the HTTP/1.1 protocol.

⚙️ **Features**

HTTP/1.1 compliance:

Supports basic HTTP methods: GET, POST, and DELETE.

Configuration file parsing:

Custom .conf file to define server blocks, ports, routes, methods, error pages, and CGI handling.

Static file serving:

Ability to serve HTML, CSS, JavaScript, images, and other assets.

Directory listing (autoindex):

If enabled in the configuration, the server dynamically generates a listing of files in a directory.

CGI support:

Execute external scripts (e.g., PHP, Python) for dynamic content generation.

Error handling:

Custom error pages (e.g., 404, 500).

Multiple clients support:

Non-blocking sockets with multiplexing (poll) allow handling multiple requests simultaneously.

Chunked transfer encoding:

Support for responses sent in chunks instead of a single block.

🛠️ **Technical Details**

Implemented in C++98, following the 42 norm.

Uses low-level socket programming (no external HTTP libraries).

Event-driven approach using I/O multiplexing for scalability.

Fully modular structure: request parsing, response building, configuration parsing, and CGI execution are handled in separate components.

📚 **Learning Outcomes**

This project provided a deep understanding of:

The HTTP protocol and how web servers communicate with browsers.

Socket programming and non-blocking I/O.

Process management and inter-process communication (for CGI).

Designing and building a modular and maintainable codebase.

Debugging complex networking applications.

Teamwork, version control with Git, and collaborative problem-solving.
