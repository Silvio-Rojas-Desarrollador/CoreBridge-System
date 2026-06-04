-- ============================================================================
-- ESQUEMA DE BASE DE DATOS - ESTÁNDAR INDUSTRIAL ACID
-- PROYECTO: Sistema de Gestión de Estudiantes (Módulo de Persistencia)
-- MOTOR: SQLite 3
-- ============================================================================

-- Habilitar explícitamente el soporte de claves foráneas y restricciones del motor
PRAGMA foreign_keys = ON;

-- Creación de la tabla principal con restricciones de integridad rígidas
CREATE TABLE IF NOT EXISTS estudiantes (
    id     INTEGER PRIMARY KEY AUTOINCREMENT,
    nombre TEXT NOT NULL CHECK(length(trim(nombre)) > 0),
    edad   INTEGER NOT NULL CHECK(edad >= 5 AND edad <= 100),
    curso  TEXT NOT NULL CHECK(length(trim(curso)) > 0),
    turno  TEXT NOT NULL CHECK(turno IN ('Mañana', 'Tarde', 'Noche'))
);

-- Indexación estratégica para optimizar lecturas concurrentes del Servidor
CREATE INDEX IF NOT EXISTS idx_estudiantes_id ON estudiantes(id);