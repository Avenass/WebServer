#!/usr/bin/env python3

import os
import sys
import urllib.parse

# Leer datos POST
content_length = int(os.environ.get('CONTENT_LENGTH', '0'))
if content_length > 0:
    post_data = sys.stdin.read(content_length)
    parsed_data = urllib.parse.parse_qs(post_data)
    message = parsed_data.get('message', ['No message'])[0]
else:
    message = "No data received"

print("Content-Type: text/html\n")
print(f"""<!DOCTYPE html>
<html>
<head>
    <title>Form Processing CGI</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 40px; }}
        h1 {{ color: #4CAF50; }}
        .result {{ background: #e8f5e8; padding: 20px; border-radius: 5px; border: 1px solid #4CAF50; }}
        .info {{ background: #f0f0f0; padding: 15px; border-radius: 5px; margin: 20px 0; }}
        code {{ background: #f5f5f5; padding: 2px 4px; border-radius: 3px; }}
    </style>
</head>
<body>
    <h1>Form Processing Result</h1>

    <div class="result">
        <h2>Message Received:</h2>
        <p><strong>"{message}"</strong></p>
    </div>

    <div class="info">
        <h3>Processing Details:</h3>
        <p><strong>Method:</strong> {os.environ.get('REQUEST_METHOD', 'Unknown')}</p>
        <p><strong>Content Length:</strong> {content_length} bytes</p>
        <p><strong>Content Type:</strong> {os.environ.get('CONTENT_TYPE', 'Not set')}</p>
        <p><strong>Raw Data:</strong> <code>{post_data if content_length > 0 else 'None'}</code></p>
    </div>

    <p><a href="/">Back to main page</a></p>
</body>
</html>""")
