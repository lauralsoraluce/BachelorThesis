import os
import subprocess
from datetime import datetime
import stat
import re

# ------------------------------------------------------------
# CONFIGURACIÓN EXPERIMENTO
# ------------------------------------------------------------
ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXEC_PATH = os.path.join(ROOT_DIR, "build", "main")
RESULTS_ROOT = os.path.join(ROOT_DIR, "results", "exp_genetico")

# parámetros de las instancias
SEEDS = [1000, 1010, 1020, 1030, 1040, 1050, 1060, 1070, 1080, 1090, 2000, 2010, 2020, 2030, 2040, 2050, 2060, 2070, 2080, 2090]
#SEEDS = [1020]
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
  ALGORITMOS: genetico (NSGA-II) + greedy

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
DELIMITER_START = "@@@REPRO_DATA_START@@@"
DELIMITER_END = "@@@REPRO_DATA_END@@@"

for i, seed in enumerate(SEEDS, start=1):
    print(f"\n>>> Ejecutando experimento {i}/{len(SEEDS)} con semilla {seed}...\n")
    cmd = [EXEC_PATH, "--no-test", "--seed", str(seed), "--k", str(K), "--Fmin", str(F_N_MIN), "--Fmax", str(F_N_MAX), "--FsizeMin", str(F_SIZE_MIN), "--FsizeMax", str(F_SIZE_MAX)]
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, cwd=ROOT_DIR, check=False,)
    
    full_output = proc.stdout or ""
    
    match = re.search(rf'{DELIMITER_START}(.*?){DELIMITER_END}', full_output, re.DOTALL)

    if match:
        repro_data = match.group(1).strip() # Datos de conjuntos (G y F)
        
        # Eliminar el bloque de reproducibilidad del output principal
        fout_data = full_output.replace(match.group(0), "").strip()
    else:
        # Si no se encuentra el bloque, todo va a fout
        repro_data = ""
        fout_data = full_output.strip()

    # 2. Escribir los datos al archivo de resultados (fout)
    with open(path_result, "a", encoding="utf-8") as fout:
        fout.write(f"\n===============================================================================\n")
        fout.write(f"EXPERIMENTO {i}/{len(SEEDS)} - SEMILLA: {seed}\n")
        fout.write("===============================================================================\n\n")
        fout.write(fout_data)
        fout.write("\n\n")

    # 3. Escribir los datos al archivo de reproducibilidad (frep)
    with open(path_repro, "a", encoding="utf-8") as frep:
        frep.write(f"\n===============================================================================\n")
        frep.write(f"EXPERIMENTO {i} - SEMILLA: {seed}\n")
        frep.write("===============================================================================\n")
        
        # Escribir el comando (esto es tu código original)
        frep.write(f"Comando para reproducir:\n")
        frep.write(
            f"  {EXEC_PATH} --no-test "
            f"--seed {seed} "
            f"--k {K} "
            f"--Fmin {F_N_MIN} --Fmax {F_N_MAX} "
            f"--FsizeMin {F_SIZE_MIN} --FsizeMax {F_SIZE_MAX}\n\n"
        )
        
        # Escribir los datos de G y F (que estaban en print_conjuntos)
        if repro_data:
            frep.write(repro_data)
            frep.write("\n")


print(f"\n✅ Experimento completado.\nResultados guardados en: {exp_dir}\n")
