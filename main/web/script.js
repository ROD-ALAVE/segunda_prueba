let ip = '192.168.1.100'; // IP por defecto

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

// Controlar salidas digitales (LED en GPIO 2)
async function controlSalida(salida, estado) {
    // Tu JS estaba enviando GPIO 2 y 4. Nosotros solo tenemos el GPIO 2 como salida.
    const gpio = 2; 
    const value = estado === 'on' ? 1 : 0;

    try {
        // El C espera /control?led=1 o /control?led=0
        const response = await fetch(`/control?led=${value}`);
        const text = await response.text(); // El C devuelve texto plano, no JSON

        if (text.includes("actualizado")) {
            actualizarUI(salida, value);
        }
    } catch (e) {
        // Simulación local si falla la conexión
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

// Controlar Salida Analógica (DAC en GPIO 25) - Antes llamado PWM
async function updatePWM(value) {
    document.getElementById('pwmSlider').value = value;
    document.getElementById('pwmValue').innerHTML = `${Math.round(value / 255 * 100)}%`;
    document.getElementById('pwmPercent').innerHTML = `${Math.round(value / 255 * 100)}%`;

    try {
        // El C espera /control?dac=0 hasta 255
        await fetch(`/control?dac=${value}`);
    } catch (e) {
        console.error('Error DAC/PWM:', e);
    }
}

// Leer entradas digitales (GPIO 12 y 13)
async function leerEntradas() {
    try {
        // El C tiene la ruta /analog que devuelve dig1, dig2, analog, dac
        const response = await fetch('/analog');
        const data = await response.json();

        // Actualizar entrada 1 (GPIO 12) -> El C lo llama "dig1"
        const val1 = data.dig1 || 0;
        document.getElementById('digital1State').innerHTML = `LECTURA: ${val1}`;
        document.getElementById('digital1Value').innerHTML = val1 === 1 ? '🔵 ALTO' : '⚫ BAJO';
        document.getElementById('digital1State').className = val1 === 1 ? 'input-high' : 'input-low';

        // Actualizar entrada 2 (GPIO 13) -> El C lo llama "dig2"
        const val2 = data.dig2 || 0;
        document.getElementById('digital2State').innerHTML = `LECTURA: ${val2}`;
        document.getElementById('digital2Value').innerHTML = val2 === 1 ? '🔵 ALTO' : '⚫ BAJO';
        document.getElementById('digital2State').className = val2 === 1 ? 'input-high' : 'input-low';
    } catch (e) {
        console.log('Usando simulación local');
        // Simulación para prueba
        const val1 = Math.random() > 0.5 ? 1 : 0;
        const val2 = Math.random() > 0.5 ? 1 : 0;
        document.getElementById('digital1State').innerHTML = `LECTURA: ${val1}`;
        document.getElementById('digital1Value').innerHTML = val1 === 1 ? '🔵 ALTO' : '⚫ BAJO';
        document.getElementById('digital2State').innerHTML = `LECTURA: ${val2}`;
        document.getElementById('digital2Value').innerHTML = val2 === 1 ? '🔵 ALTO' : '⚫ BAJO';
    }
}

// Leer entrada analógica (GPIO 34)
async function leerAnalogico() {
    try {
        // El C responde en /analog con un JSON que contiene "analog"
        const response = await fetch('/analog');
        const data = await response.json();
        const valor = data.analog || 0;
        const porcentaje = (valor / 4095 * 100).toFixed(1);

        document.getElementById('analogValue').innerHTML = `${valor} / 4095`;
        document.getElementById('analogText').innerHTML = `${valor}`;
        document.getElementById('analogPercent').innerHTML = `${porcentaje}%`;
    } catch (e) {
        // Simulación
        const sim = Math.floor(Math.random() * 4096);
        const porc = (sim / 4095 * 100).toFixed(1);
        document.getElementById('analogValue').innerHTML = `${sim} / 4095`;
        document.getElementById('analogText').innerHTML = `${sim}`;
        document.getElementById('analogPercent').innerHTML = `${porc}%`;
    }
}

// Actualizar todos los datos
async function actualizarTodo() {
    await obtenerIP();
    await leerEntradas();
    await leerAnalogico();
}

// Auto-actualización cada 500ms
setInterval(actualizarTodo, 500);

// Inicializar
actualizarTodo();

// Para la salida PWM (DAC), asegurar que se envía al soltar el slider
let timeoutPWM;
document.getElementById('pwmSlider').addEventListener('change', function (e) {
    updatePWM(this.value);
});