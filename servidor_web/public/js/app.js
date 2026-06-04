/**
 * APP.JS - Lógica de la SPA (Single Page Application)
 * Gestiona el ciclo de vida de la sesión, la comunicación asíncrona con el Gateway
 * y la renderización dinámica de datos provenientes del Broker en C.
 */

const API_URL = '/api';

// --- ESTADO GLOBAL DE LA APLICACIÓN ---
const state = {
    token: localStorage.getItem('gateway_token'),
    user: localStorage.getItem('gateway_user'),
};

// --- SELECTORES DOM ---
const dom = {
    loginView: document.getElementById('login-view'),
    dashboardView: document.getElementById('dashboard-view'),
    loginForm: document.getElementById('login-form'),
    loginError: document.getElementById('login-error'),
    studentsBody: document.getElementById('students-body'),
    userDisplay: document.getElementById('user-display'),
    btnLogout: document.getElementById('btn-logout'),
    btnRefresh: document.getElementById('btn-refresh')
};

// --- FUNCIONES DE NAVEGACIÓN Y VISTA ---
const updateView = () => {
    if (state.token) {
        dom.loginView.classList.add('hidden');
        dom.dashboardView.classList.remove('hidden');
        dom.userDisplay.textContent = state.user;
        fetchStudents();
    } else {
        dom.loginView.classList.remove('hidden');
        dom.dashboardView.classList.add('hidden');
        dom.studentsBody.innerHTML = '';
    }
};

const showError = (message) => {
    dom.loginError.textContent = message;
    dom.loginError.classList.remove('hidden');
    setTimeout(() => dom.loginError.classList.add('hidden'), 5000);
};

// --- LÓGICA DE NEGOCIO (API CONSUMPTION) ---

/**
 * Gestiona el inicio de sesión contra el Gateway.
 */
async function handleLogin(e) {
    e.preventDefault();
    const usuario = document.getElementById('username').value;
    const password = document.getElementById('password').value;

    try {
        const response = await fetch(`${API_URL}/auth/login`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ usuario, password })
        });

        const result = await response.json();

        if (response.ok && result.token) {
            state.token = result.token;
            state.user = usuario;
            localStorage.setItem('gateway_token', result.token);
            localStorage.setItem('gateway_user', usuario);
            updateView();
        } else {
            showError(result.mensaje || 'Credenciales rechazadas por el Broker.');
        }
    } catch (error) {
        showError('Error crítico: El Gateway no responde.');
    }
}

/**
 * Recupera la lista de estudiantes del Broker (Solo Lectura).
 */
async function fetchStudents() {
    try {
        const response = await fetch(`${API_URL}/students`, {
            headers: { 
                'Authorization': `Bearer ${state.token}` 
            }
        });

        if (response.status === 401 || response.status === 403) {
            handleLogout();
            return;
        }

        const result = await response.json();
        renderTable(result.data || []);
    } catch (error) {
        console.error('Error al sincronizar con el Broker:', error);
    }
}

/**
 * Renderiza los registros binarios transformados en la tabla HTML.
 */
function renderTable(students) {
    dom.studentsBody.innerHTML = students.map(s => `
        <tr>
            <td><strong>${s.id_estudiante}</strong></td>
            <td>${s.nombre_estudiante}</td>
            <td>${s.edad} años</td>
            <td><span class="tag">${s.curso}</span></td>
            <td>${s.turno}</td>
            <td>
                <div class="progress-bar">
                    <div class="progress" style="width: ${s.asistencia}%"></div>
                    <span>${s.asistencia}%</span>
                </div>
            </td>
        </tr>
    `).join('');

    if (students.length === 0) {
        dom.studentsBody.innerHTML = '<tr><td colspan="6" class="empty-msg">No hay registros disponibles en la base de datos central.</td></tr>';
    }
}

/**
 * Cierre de sesión y limpieza de seguridad.
 */
function handleLogout() {
    state.token = null;
    state.user = null;
    localStorage.removeItem('gateway_token');
    localStorage.removeItem('gateway_user');
    updateView();
}

// --- EVENT LISTENERS ---
dom.loginForm.addEventListener('submit', handleLogin);
dom.btnLogout.addEventListener('click', handleLogout);
dom.btnRefresh.addEventListener('click', fetchStudents);

// --- INICIALIZACIÓN ---
// Verificar si hay una sesión activa al cargar la página
window.addEventListener('DOMContentLoaded', updateView);