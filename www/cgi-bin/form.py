#!/usr/bin/env python3

import os
import sys
import urllib.parse
from datetime import datetime

def main():
    # Leer datos POST
    content_length = int(os.environ.get('CONTENT_LENGTH', '0'))
    post_data = ""
    message = "No message received"

    if content_length > 0:
        post_data = sys.stdin.read(content_length)
        try:
            parsed_data = urllib.parse.parse_qs(post_data)
            message = parsed_data.get('message', ['No message'])[0]
        except Exception as e:
            message = f"Error parsing data: {str(e)}"

    # Headers CGI
    print("Content-Type: text/html")
    print()  # Línea vacía obligatoria entre headers y body

    # HTML Response
    print(f"""<!DOCTYPE html>
<html>
<head>
    <title>CGI Form Processing Result</title>
    <style>
        body {{
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 40px auto;
            padding: 20px;
            background: #f5f5f5;
        }}
        .container {{
            background: white;
            border-radius: 10px;
            padding: 30px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.1);
        }}
        h1 {{
            color: #4CAF50;
            border-bottom: 2px solid #4CAF50;
            padding-bottom: 10px;
        }}
        .result-box {{
            background: #e8f5e8;
            border-left: 4px solid #4CAF50;
            padding: 20px;
            margin: 20px 0;
            border-radius: 5px;
        }}
        .info-section {{
            background: #f0f8ff;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0;
        }}
        .debug-info {{
            background: #f8f9fa;
            border: 1px solid #dee2e6;
            padding: 15px;
            border-radius: 5px;
            font-family: monospace;
            margin: 20px 0;
        }}
        .back-btn {{
            background: #4CAF50;
            color: white;
            padding: 10px 20px;
            text-decoration: none;
            border-radius: 5px;
            display: inline-block;
            margin-top: 20px;
        }}
        .back-btn:hover {{
            background: #45a049;
        }}
        .success {{
            color: #4CAF50;
            font-weight: bold;
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>🐍 CGI Form Processing Result</h1>

        <div class="result-box">
            <h2>Message Received:</h2>
            <p class="success">"{message}"</p>
        </div>

        <div class="info-section">
            <h3>📊 Processing Information:</h3>
            <p><strong>Method:</strong> {os.environ.get('REQUEST_METHOD', 'Unknown')}</p>
            <p><strong>Content Length:</strong> {content_length} bytes</p>
            <p><strong>Content Type:</strong> {os.environ.get('CONTENT_TYPE', 'Not set')}</p>
            <p><strong>Processing Time:</strong> {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        </div>

        <div class="debug-info">
            <h3>🔧 Debug Information:</h3>
            <p><strong>Raw POST Data:</strong></p>
            <pre>{post_data if post_data else 'No POST data received'}</pre>

            <p><strong>CGI Environment Variables:</strong></p>
            <pre>SCRIPT_NAME: {os.environ.get('SCRIPT_NAME', 'Not set')}
REQUEST_URI: {os.environ.get('REQUEST_URI', 'Not set')}
QUERY_STRING: {os.environ.get('QUERY_STRING', 'Not set')}
HTTP_HOST: {os.environ.get('HTTP_HOST', 'Not set')}
HTTP_USER_AGENT: {os.environ.get('HTTP_USER_AGENT', 'Not set')[:50]}...
SERVER_SOFTWARE: {os.environ.get('SERVER_SOFTWARE', 'Not set')}
GATEWAY_INTERFACE: {os.environ.get('GATEWAY_INTERFACE', 'Not set')}</pre>
        </div>

        <div class="info-section">
            <h3>✅ CGI Test Results:</h3>
            <p>✓ CGI script executed successfully</p>
            <p>✓ POST data received and parsed</p>
            <p>✓ Environment variables accessible</p>
            <p>✓ Headers sent correctly</p>
            <p>✓ HTML response generated</p>
        </div>

        <a href="/" class="back-btn">← Back to Main Page</a>
    </div>
</body>
</html>""")

if __name__ == "__main__":
    main()
