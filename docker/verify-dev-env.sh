#!/usr/bin/env bash
# Script de verificación para el entorno Docker de desarrollo Gestor-Horarios
# Uso: bash docker/verify-dev-env.sh
# Requiere: Docker running, docker compose installed

set -euo pipefail

PASS=0
FAIL=0

green() { echo -e "\033[32m✓ $1\033[0m"; }
red() { echo -e "\033[31m✗ $1\033[0m"; }

echo "=== Gestor-Horarios Dev Env Verification ==="
echo ""

# ── 3.1 Build ──
echo "--- 3.1: Building container ---"
if docker compose -f docker/docker-compose.yml build 2>&1 | tail -5; then
    green "Build completed successfully"
    PASS=$((PASS + 1))
else
    red "Build failed"
    FAIL=$((FAIL + 1))
fi

echo ""

# ── 3.2 OR-Tools detection ──
echo "--- 3.2: Verifying OR-Tools (cmake --find-package) ---"
if docker compose -f docker/docker-compose.yml run --rm dev cmake --find-package \
    -DNAME=ortools -DCOMPILER_ID=GNU -DLANGUAGE=CXX 2>&1; then
    green "OR-Tools found by CMake"
    PASS=$((PASS + 1))
else
    red "OR-Tools not found by CMake"
    FAIL=$((FAIL + 1))
fi

echo ""

# ── 3.3 Qt6 ──
echo "--- 3.3: Verifying Qt6 (qmake6 --version) ---"
if docker compose -f docker/docker-compose.yml run --rm dev qmake6 --version 2>&1; then
    green "Qt6 is available"
    PASS=$((PASS + 1))
else
    red "Qt6 not available"
    FAIL=$((FAIL + 1))
fi

echo ""

# ── 3.4 Build tools ──
echo "--- 3.4: Verifying build tools ---"
if docker compose -f docker/docker-compose.yml run --rm dev sh -c \
    "cmake --version && echo '---' && ninja --version" 2>&1; then
    green "Build tools (cmake + ninja) working"
    PASS=$((PASS + 1))
else
    red "Build tools not working"
    FAIL=$((FAIL + 1))
fi

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="
exit $FAIL
