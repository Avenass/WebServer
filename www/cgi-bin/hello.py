#!/usr/bin/env python3

print("Content-Type: text/html\n")
print("""<!DOCTYPE html>
<html>
<head>
    <title>Hello CGI</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; text-align: center; }
        h1 { color: #4CAF50; }
        .info { background: #f0f0f0; padding: 20px; border-radius: 5px; margin: 20px auto; max-width: 500px; }
    </style>
</head>
<body>
    <h1>Hello from Python CGI!</h1>
    <div class="info">
        <p><strong>CGI Script:</strong> hello.py</p>
        <p><strong>Server:</strong> webserv/1.0</p>
        <p><strong>Method:</strong> GET</p>
        <p>This proves that CGI execution is working correctly!</p>
    </div>
    <p><a href="/"> Back to main page</a></p>
</body>
</html>""")
