/**
 * INDEX.JS - Punto de Entrada del API Gateway
 * Orquestador principal encargado de levantar el servicio Express,
 * gestionar middlewares y configurar las rutas de consumo para el Frontend.
 */

const express = require('express');
const cors = require('cors');
const path = require('path');
require('dotenv').config();

const app = express();
const PORT = process.env.PORT || 3000;

// 1. Middlewares de Infraestructura
app.use(cors());
app.use(express.json()); // Permite procesar payloads JSON del frontend

// 2. Servicio de Archivos Estáticos (SPA)
app.use(express.static(path.join(__dirname, '../public')));

// 3. Definición de Rutas (Endpoints)
// Nota: Los controladores se activarán a medida que desarrollemos sus respectivos archivos
const authController = require('./controllers/authController');
const studentController = require('./controllers/studentController');
const authMiddleware = require('./middleware/auth');

app.post('/api/auth/login', authController.login);
app.get('/api/students', authMiddleware, studentController.getAll);

// 4. Fallback para Single Page Application
app.get('*', (req, res) => {
    res.sendFile(path.join(__dirname, '../public/index.html'));
});

// 5. Lanzamiento del Proceso
app.listen(PORT, () => {
    console.log('=================================================');
    console.log(`  GATEWAY INDUSTRIAL - STANDBY EN PUERTO ${PORT}`);
    console.log('  SISTEMA HÍBRIDO C/NODE.JS - NIVEL SENIOR      ');
    console.log('=================================================');
});