# Proyecto Servidor Híbrido: Ecosistema de Mensajería y Datos (MIT Level)

## 📌 Visión General
Este proyecto representa una arquitectura híbrida de alto rendimiento que combina la eficiencia de bajo nivel de **C (Core Broker)** con la flexibilidad de las tecnologías **Web (API Gateway)**. El sistema permite la gestión de bases de datos relacionales (SQLite) y mensajería en tiempo real a través de múltiples protocolos.

## 🏗️ Plano Arquitectónico del Sistema

```text
[ CAPA 1: PRESENTACIÓN (Frontend) ]
       │
       ▼ (HTTP/JSON o WebSockets)

[ CAPA 2: SERVIDOR HÍBRIDO / API GATEWAY ]  <-- (Fase Actual de Extensión)
       │   • Gestión de Sesiones (JWT/Tokens)
       │   • Traducción: JSON <──> PacketRed (284 bytes)
       │
       ▼ (TCP Binario - Localhost:8080)

[ CAPA 3: MESSAGE BROKER (Core C) ]
       │   • Gestión de Concurrencia (Multithreading)
       │   • Ruteo de Mensajería (Privados/Broadcast)
       │
       ▼ (Lógica de Persistencia)

[ CAPA 4: DATOS (SQLite & Binary Files) ]
       • archivo.db (Estudiantes)
       • usuarios.dat (Credenciales)
```

## 📂 Disposición de Carpetas

```text
servidor_hibrido/
│
├── README.md                 # Este archivo (Master Plan)
│
├── servidor_web/             # CAPA GATEWAY: Node.js / Express
│   ├── src/
│   │   ├── index.js          # Punto de entrada y configuración de Express
│   │   ├── bridge/           # Motor de Traducción Binaria
│   │   │   ├── serializer.js # Conversión de JSON a Buffers (PacketRed 284 bytes)
│   │   │   └── opcodes.js    # Definición de códigos de operación (Espejo de C)
│   │   ├── controllers/      # Lógica de Controladores
│   │   │   ├── authController.js    # Manejo de Login y Registro
│   │   │   └── studentController.js # Manejo de CRUD de Estudiantes
│   │   ├── middleware/       # Filtros de Seguridad
│   │   │   └── auth.js       # Validación de JWT para rutas protegidas
│   │   └── network/          # Comunicación de Bajo Nivel
│   │       └── tcpClient.js  # Gestión de conexión persistente con el Broker C
│   ├── public/               # Frontend (Single Page Application)
│   │   ├── index.html        # Estructura principal
│   │   ├── js/app.js         # Lógica de consumo de API interna
│   │   └── css/styles.css    # Estilos de la interfaz
│   └── package.json          # Dependencias y configuración del entorno
│
├── servidor_broker/          # NÚCLEO: Sistema Message Broker (Actualizado)
│   ├── mi_servidor/          # Daemon C con threading industrial
│   ├── mi_administrador/     # Herramienta de gestión local
│   ├── cliente_uno/          # Cliente de prueba asíncrono
│   └── datos/                # Almacén compartido de persistencia (SQLite/DAT)
│
└── docs/                     # Especificaciones técnicas del protocolo PacketRed
```

## 🔐 Estrategia de Seguridad e Integridad
1. **Single Source of Truth (SSoT):** La base de datos y el archivo de credenciales residen exclusivamente bajo el control del `servidor_broker`.
2. **Aislamiento de Red:** El Broker no escucha peticiones externas directas; solo acepta conexiones del Gateway local o clientes autorizados en la VPN.
3. **Alineación de Memoria:** Se garantiza el uso de `#pragma pack(1)` en todos los módulos para asegurar que el `PacketRed` sea interpretado idénticamente por el Gateway y el Broker.

## 🚀 Flujo de Operación (Ejemplo: Crear Estudiante)
1. El usuario envía un formulario Web (JSON).
2. El `servidor_web` valida el Token de sesión.
3. El Gateway construye un struct `PacketRed` con `tipo_operacion = OP_CREAR_ESTUDIANTE`.
4. El Gateway envía el bloque de bytes vía socket TCP al `servidor_broker`.
5. El `servidor_broker` ejecuta la transacción en SQLite y responde con `ESTADO_EXITO`.
6. El Gateway recibe los bytes, los transforma a JSON y responde al navegador con código 201.

---
**Estado del Proyecto:** Fase de Planeación (Híbrido v2.0)  
**Lead Architect:** Senior Elite Engineer / MIT Standard
```

He estructurado este plano para que el `servidor_broker` (que ya tienes avanzado) no necesite cambios drásticos, sino que se convierta en el motor que alimenta la web. La separación en capas nos permitirá escalar el frontend sin poner en riesgo la integridad de la base de datos en C.

¿Qué te parece la estructura de carpetas propuesta para `servidor_web`? ¿Deseas que profundicemos en el sub-módulo `bridge/` que hará la traducción binaria?

<!--
[PROMPT_SUGGESTION]Define los requerimientos técnicos del sub-módulo bridge para conectar el servidor web con el broker en C.[/PROMPT_SUGGESTION]
[PROMPT_SUGGESTION]¿Qué tecnología de servidor web (Node.js, Go o C con Mongoose) se adapta mejor a nuestra estructura de PacketRed?[/PROMPT_SUGGESTION]
