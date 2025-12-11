<?php
// Mostrar errores para depuración
error_reporting(E_ALL);
ini_set('display_errors', 1);

// --- Conexión a la base de datos ---
$servername = "localhost";
$username = "root";
$password = "";
$dbname = "sensores_db";

$conn = new mysqli($servername, $username, $password, $dbname);
if ($conn->connect_error) {
    die(json_encode(array("status" => "error", "message" => "Error de conexión: " . $conn->connect_error)));
}

// --- Leer datos JSON recibidos ---
$data = file_get_contents("php://input");
$json = json_decode($data, true);

if (!$json) {
    echo json_encode(array("status" => "error", "message" => "JSON inválido o vacío"));
    exit;
}

// --- Extraer variables del JSON ---
$temperatura = $json["temperatura"];
$humedad = $json["humedad"];
$gas = isset($json["gas"]) ? $json["gas"] : $json["voltaje_gas"];
$ruido = ($json["ruido"] == 1) ? "Ruido detectado" : "Ambiente silencioso";
$latitud = $json["latitud"];
$longitud = $json["longitud"];

// --- Preparar consulta SQL ---
$sql = "INSERT INTO lecturas (temperatura, humedad, concentracion_gas, Ruido, latitud, longitud)
        VALUES (?, ?, ?, ?, ?, ?)";

$stmt = $conn->prepare($sql);
if (!$stmt) {
    echo json_encode(array("status" => "error", "message" => "Error en prepare(): " . $conn->error));
    exit;
}

// dddsdd → double, double, double, string, double, double
$stmt->bind_param("dddsdd", $temperatura, $humedad, $gas, $ruido, $latitud, $longitud);

// --- Ejecutar e informar resultado ---
if ($stmt->execute()) {
    echo json_encode(array("status" => "ok", "message" => "Datos insertados correctamente"));
} else {
    echo json_encode(array("status" => "error", "message" => "Error al insertar datos: " . $stmt->error));
}

// --- Cerrar conexiones ---
$stmt->close();
$conn->close();
?>
