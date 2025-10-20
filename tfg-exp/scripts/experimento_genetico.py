import os
import subprocess
from datetime import datetime
import stat

# ------------------------------------------------------------
# CONFIGURACIÓN EXPERIMENTO
# ------------------------------------------------------------
ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXEC_PATH = os.path.join(ROOT_DIR, "build", "main")
RESULTS_ROOT = os.path.join(ROOT_DIR, "results", "exp_genetico")

# parámetros de las instancias
SEEDS = [1020, 1030, 1040, 1050, 1060, 1070, 1080, 1090, 2000, 2001, 2002, 2003, 2004, 2005]
U_SIZE = 128
F_N_MIN, F_N_MAX = 5, 100
F_SIZE_MIN, F_SIZE_MAX = 5, 100
K = 10

# ------------------------------------------------------------
# PREPARAR DIRECTORIOS
# ------------------------------------------------------------
timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
exp_dir = os.path.join(RESULTS_ROOT, timestamp)
os.makedirs(exp_dir, exist_ok=True)

path_repro = os.path.join(exp_dir, "reproducibilidad.txt")
path_result = os.path.join(exp_dir, "resultados.txt")

# ------------------------------------------------------------
# CABECERA ARCHIVOS
# ------------------------------------------------------------
fecha = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
header_repro = f"""===============================================================================
DATOS DE REPRODUCIBILIDAD - EXP_GENETICO
Fecha: {fecha}
===============================================================================

PARÁMETROS DE CONFIGURACIÓN:
  U_SIZE: {U_SIZE}
  F_N_MIN: {F_N_MIN}
  F_N_MAX: {F_N_MAX}
  F_SIZE_MIN: {F_SIZE_MIN}
  F_SIZE_MAX: {F_SIZE_MAX}
  K: {K}
  SEMILLAS: {SEEDS}
  ALGORITMOS: genetico (NSGA-II)

===============================================================================
"""

with open(path_repro, "w", encoding="utf-8") as f:
    f.write(header_repro)

with open(path_result, "w", encoding="utf-8") as f:
    f.write("===============================================================================\n")
    f.write("RESULTADOS - EXPERIMENTO EXP_GENETICO\n")
    f.write(f"Fecha: {fecha}\n")
    f.write("===============================================================================\n\n")
    
# ------------------------------------------------------------
# BUCLE PRINCIPAL DE EXPERIMENTOS
# ------------------------------------------------------------
for i, seed in enumerate(SEEDS, start=1):
    print(f"\n>>> Ejecutando experimento {i}/{len(SEEDS)} con semilla {seed}...\n")

    with open(path_result, "a", encoding="utf-8") as fout:
        fout.write(f"\n===============================================================================\n")
        fout.write(f"EXPERIMENTO {i}/{len(SEEDS)} - SEMILLA: {seed}\n")
        fout.write("===============================================================================\n\n")

        # Ejecutar el programa C++ (pasa la semilla como argumento)
        cmd = [EXEC_PATH, "false", str(seed)]
        proc = subprocess.run(cmd, stdout=fout, stderr=subprocess.STDOUT, text=True, cwd=ROOT_DIR)

        fout.write(f"\n[returncode] {proc.returncode}\n\n")
        fout.write("\n\n")

    # reproducibilidad.txt
    with open(path_repro, "a", encoding="utf-8") as frep:
        frep.write(f"\n===============================================================================\n")
        frep.write(f"EXPERIMENTO {i} - SEMILLA: {seed}\n")
        frep.write("===============================================================================\n")
        frep.write(f"Comando para reproducir:\n  {EXEC_PATH} false {seed}\n\n")

print(f"\n✅ Experimento completado.\nResultados guardados en: {exp_dir}\n")

