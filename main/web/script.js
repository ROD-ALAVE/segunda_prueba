let ip = '192.168.0.100'; // IP por defecto

// Obtener IP del ESP32
async function obtenerIP() {
    try {
        const response = await fetch('/ip');
        const data = await response.json();
        if (data.ip) {
            ip = data.ip;
            document.getElementById('ipDisplay').innerHTML = `🌐 IP: ${ip}`;
        }
    } catch (e) {
        console.log('Usando IP por defecto');
    }
}

// Controlar salidas digitales (LED en GPIO 2 y GPIO 4)
async function controlSalida(salida, estado) {
    const gpioMap = {
        1: 2,    // GPIO2
        2: 4     // GPIO4
    };

    const gpio = gpioMap[salida];
    if (!gpio) {
        console.error('Salida no válida:', salida);
        return;
    }

    const value = estado === 'on' ? 1 : 0;
    const nombreControl = salida === 1 ? 'led' : 'led4';

    try {
        const response = await fetch(`/control?${nombreControl}=${value}`);
        const text = await response.text();

        if (text.includes("actualizado")) {
            actualizarUI(salida, value);
        }
    } catch (e) {
        actualizarUI(salida, value);
        console.error('Error:', e);
    }
}

// Actualizar UI localmente
function actualizarUI(salida, value) {
    const isOn = value === 1;
    const statusSpan = document.getElementById(`led${salida}Status`);
    const textSpan = document.getElementById(`led${salida}Text`);
    const estadoTexto = isOn ? 'ENCENDIDO' : 'APAGADO';

    if (statusSpan) {
        statusSpan.className = `status-led ${isOn ? 'status-on' : 'status-off'}`;
    }
    if (textSpan) {
        textSpan.textContent = estadoTexto;
    }
}

// Controlar salida analógica DAC en GPIO 25
async function updateDAC(value) {
    document.getElementById('pwmSlider').value = value;
    document.getElementById('pwmValue').innerHTML = `${Math.round(value / 255 * 100)}%`;
    document.getElementById('pwmPercent').innerHTML = `${Math.round(value / 255 * 100)}%`;

    try {
        await fetch(`/control?dac=${value}`);
    } catch (e) {
        console.error('Error DAC:', e);
    }
}

// Controlar salida PWM en GPIO 26
async function updatePWM(value) {
    document.getElementById('pwmOutputSlider').value = value;
    document.getElementById('pwmOutputValue').innerHTML = `${Math.round(value / 255 * 100)}%`;

    try {
        await fetch(`/control?pwm=${value}`);
    } catch (e) {
        console.error('Error PWM:', e);
    }
}

// Leer TODO desde /analog en una sola petición: entradas digitales + entrada analógica
async function leerDatos() {
    try {
        const response = await fetch('/analog');
        const data = await response.json();

        // Entrada digital 1 (GPIO 12)
        const val1 = data.dig1 || 0;
        document.getElementById('digital1State').innerHTML = `LECTURA: ${val1}`;
        document.getElementById('digital1Value').innerHTML = val1 === 1 ? '🔵 ALTO' : '⚫ BAJO';
        document.getElementById('digital1State').className = val1 === 1 ? 'input-high' : 'input-low';

        // Entrada digital 2 (GPIO 13)
        const val2 = data.dig2 || 0;
        document.getElementById('digital2State').innerHTML = `LECTURA: ${val2}`;
        document.getElementById('digital2Value').innerHTML = val2 === 1 ? '🔵 ALTO' : '⚫ BAJO';
        document.getElementById('digital2State').className = val2 === 1 ? 'input-high' : 'input-low';

        // Entrada analógica (GPIO 34)
        const valor = data.analog || 0;
        const porcentaje = (valor / 4095 * 100).toFixed(1);
        document.getElementById('analogValue').innerHTML = `${valor} / 4095`;
        document.getElementById('analogText').innerHTML = `${valor}`;
        document.getElementById('analogPercent').innerHTML = `${porcentaje}%`;

        // Segunda entrada analógica (GPIO 35)
        const valor2 = data.analog2 || 0;
        const porcentaje2 = (valor2 / 4095 * 100).toFixed(1);
        document.getElementById('analog2Value').innerHTML = `${valor2} / 4095`;
        document.getElementById('analog2Text').innerHTML = `${valor2}`;
        document.getElementById('analog2Percent').innerHTML = `${porcentaje2}%`;
    } catch (e) {
        console.log('Error leyendo /analog:', e);
    }
}

// Actualizar todos los datos (una sola petición por ciclo)
async function actualizarTodo() {
    await leerDatos();
}

// Auto-actualización cada 1000ms
setInterval(actualizarTodo, 1000);

// Inicializar
obtenerIP();
actualizarTodo();

// Para la salida PWM (DAC), asegurar que se envía al soltar el slider
document.getElementById('pwmSlider').addEventListener('change', function (e) {
    updateDAC(this.value);
});

document.getElementById('pwmOutputSlider').addEventListener('change', function (e) {
    updatePWM(this.value);
});