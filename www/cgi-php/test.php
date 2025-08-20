<?php



$server_software = $_SERVER['SERVER_SOFTWARE'] ?? 'webserv/1.0';
$request_method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
$server_protocol = $_SERVER['SERVER_PROTOCOL'] ?? 'HTTP/1.1';
$gateway_interface = $_SERVER['GATEWAY_INTERFACE'] ?? 'CGI/1.1';


$post_data = [];
if ($request_method === 'POST') {
    $post_data = $_POST;
}


$get_data = $_GET;

?>
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>PHP CGI Test - Webserv</title>
    <style>
        body {
            font-family: 'Courier New', monospace;
            background: #1e1e1e;
            color: #d4d4d4;
            padding: 20px;
            margin: 0;
        }
        .container {
            max-width: 1000px;
            margin: 0 auto;
            background: #2d2d2d;
            border-radius: 10px;
            padding: 30px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.5);
        }
        h1 {
            color: #4ec9b0;
            border-bottom: 2px solid #4ec9b0;
            padding-bottom: 10px;
        }
        .success {
            background: #27ae60;
            color: white;
            padding: 10px;
            border-radius: 5px;
            margin: 20px 0;
        }
        .info-section {
            background: #1e1e1e;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0;
            border-left: 4px solid #4ec9b0;
        }
        .info-section h2 {
            color: #569cd6;
            margin-top: 0;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin: 10px 0;
        }
        th, td {
            padding: 8px;
            text-align: left;
            border-bottom: 1px solid #444;
        }
        th {
            color: #4ec9b0;
            background: #1e1e1e;
        }
        .back-btn {
            display: inline-block;
            margin-top: 20px;
            padding: 10px 20px;
            background: #569cd6;
            color: white;
            text-decoration: none;
            border-radius: 5px;
        }
        .back-btn:hover {
            background: #4a7fb8;
        }
        .code-block {
            background: #1e1e1e;
            padding: 10px;
            border-radius: 5px;
            overflow-x: auto;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 PHP CGI Ejecutado Exitosamente</h1>

        <div class="success">
            ✅ El script PHP se ha ejecutado correctamente a través del CGI
        </div>

        <div class="info-section">
            <h2>📊 Información del Servidor</h2>
            <table>
                <tr>
                    <th>Variable</th>
                    <th>Valor</th>
                </tr>
                <tr>
                    <td>SERVER_SOFTWARE</td>
                    <td><?php echo htmlspecialchars($server_software); ?></td>
                </tr>
                <tr>
                    <td>REQUEST_METHOD</td>
                    <td><?php echo htmlspecialchars($request_method); ?></td>
                </tr>
                <tr>
                    <td>SERVER_PROTOCOL</td>
                    <td><?php echo htmlspecialchars($server_protocol); ?></td>
                </tr>
                <tr>
                    <td>GATEWAY_INTERFACE</td>
                    <td><?php echo htmlspecialchars($gateway_interface); ?></td>
                </tr>
                <tr>
                    <td>PHP_VERSION</td>
                    <td><?php echo PHP_VERSION; ?></td>
                </tr>
                <tr>
                    <td>CURRENT_TIME</td>
                    <td><?php echo date('Y-m-d H:i:s'); ?></td>
                </tr>
            </table>
        </div>

        <?php if (!empty($get_data)): ?>
        <div class="info-section">
            <h2>📥 Datos GET Recibidos</h2>
            <div class="code-block">
                <?php foreach ($get_data as $key => $value): ?>
                    <p><?php echo htmlspecialchars($key); ?> = <?php echo htmlspecialchars($value); ?></p>
                <?php endforeach; ?>
            </div>
        </div>
        <?php endif; ?>

        <?php if (!empty($post_data)): ?>
        <div class="info-section">
            <h2>📤 Datos POST Recibidos</h2>
            <div class="code-block">
                <?php foreach ($post_data as $key => $value): ?>
                    <p><?php echo htmlspecialchars($key); ?> = <?php echo htmlspecialchars($value); ?></p>
                <?php endforeach; ?>
            </div>
        </div>
        <?php endif; ?>

        <div class="info-section">
            <h2>🔧 Variables de Entorno CGI</h2>
            <div class="code-block">
                <?php
                $cgi_vars = ['PATH_INFO', 'QUERY_STRING', 'CONTENT_TYPE', 'CONTENT_LENGTH',
                            'SCRIPT_NAME', 'REQUEST_URI', 'DOCUMENT_ROOT', 'SERVER_ADMIN',
                            'SCRIPT_FILENAME', 'REMOTE_ADDR', 'REMOTE_PORT', 'SERVER_NAME',
                            'SERVER_PORT', 'SERVER_ADDR'];

                foreach ($cgi_vars as $var) {
                    if (isset($_SERVER[$var])) {
                        echo "<p><strong>$var:</strong> " . htmlspecialchars($_SERVER[$var]) . "</p>";
                    }
                }
                ?>
            </div>
        </div>

        <div class="info-section">
            <h2>📝 Test de Funcionalidad</h2>
            <p>✓ CGI Gateway funcionando</p>
            <p>✓ Variables de entorno pasadas correctamente</p>
            <p>✓ Procesamiento PHP activo</p>
            <p>✓ Salida HTML generada dinámicamente</p>
        </div>

        <a href="/" class="back-btn">← Volver al Inicio</a>
        <img src="images/image.png">
    </div>
</body>
</html>
