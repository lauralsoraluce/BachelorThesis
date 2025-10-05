#!/usr/bin/env bash
# ==========================================
# Script para compilar y ejecutar pruebas
# Búsqueda exhaustiva TFG - Laura Lázaro
# ==========================================

set -euo pipefail

# --- Configuración ---
SRC="exhaustiva.cpp"     # archivo fuente
OUT="exhaustiva"         # ejecutable
OUTFILE="resultados.txt" # fichero donde se guardan los resultados
CXX="g++"                # compilador

# --- Comprobaciones básicas ---
if ! command -v "$CXX" >/dev/null 2>&1; then
  echo "❌ No se encontró $CXX en el PATH."
  exit 1
fi

if [ ! -f "$SRC" ]; then
  echo "❌ No existe el archivo fuente: $SRC"
  exit 1
fi

# --- Compilar ---
echo "🔧 Compilando $SRC..."
"$CXX" -std=c++17 -O2 -Wall -Wextra -o "$OUT" "$SRC"
echo "✅ Compilación correcta."

# --- Inicializar archivo de resultados ---
echo "🧾 Guardando resultados en $OUTFILE"
echo "==== RESULTADOS ====" > "$OUTFILE"
date >> "$OUTFILE"
echo >> "$OUTFILE"

METRICS=(jaccard precision size)
SEEDS=(123)

N_VALUES=(6)
K_VALUES=(3 4)

for n in "${N_VALUES[@]}"; do
  for k in "${K_VALUES[@]}"; do
    for seed in "${SEEDS[@]}"; do
    	for metric in "${METRICS[@]}"; do
	      echo "▶️ Ejecutando con n=$n, k=$k, seed=$seed, metric=$metric"
	      "./$OUT" --n "$n" --k "$k" --seed "$seed" --metric "$metric" --txt "$OUTFILE"
    	done
    done
  done
done


# --- Barrido de parámetros ---
N_VALUES=(8 10 12 14 16 18 20)
K_VALUES=(3)

for n in "${N_VALUES[@]}"; do
  for k in "${K_VALUES[@]}"; do
    for seed in "${SEEDS[@]}"; do
    	for metric in "${METRICS[@]}"; do
	      echo "▶️ Ejecutando con n=$n, k=$k, seed=$seed, metric=$metric"
	      "./$OUT" --n "$n" --k "$k" --seed "$seed" --metric "$metric" --txt "$OUTFILE"
	done
    done
  done
done

echo
echo "✅ Pruebas completadas. Resultados añadidos a $OUTFILE"

