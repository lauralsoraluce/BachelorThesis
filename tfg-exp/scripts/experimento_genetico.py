import os
import subprocess
import sys
import yaml
from datetime import datetime
from pathlib import Path
import stat
import re

DELIMITER_START = "@@@REPRO_DATA_START@@@"
DELIMITER_END = "@@@REPRO_DATA_END@@@"

def cargar_configuracion(project_root):
    """Carga la configuración desde el archivo YAML en la raíz"""
    config_path = project_root / 'config.yaml'
    try:
        with open(config_path, 'r', encoding='utf-8') as f:
            config = yaml.safe_load(f)
        print(f"✓ Configuración cargada desde: {config_path}")
        return config
    except FileNotFoundError:
        print(f"Error: No se encontró el archivo {config_path}")
        sys.exit(1)
    except yaml.YAMLError as e:
        print(f"Error al parsear YAML: {e}")
        sys.exit(1)

def main():
    # ------------------------------------------------------------
    # CONFIGURACIÓN (AHORA DESDE YAML)
    # ------------------------------------------------------------
    SCRIPT_DIR = Path(__file__).resolve().parent
    PROJECT_ROOT = SCRIPT_DIR.parent
    
    config = cargar_configuracion(PROJECT_ROOT)
    
    try:
        cfg = config['genetico_config']
        paths_cfg = config['paths']
        U_SIZE = config['U_size']
    except KeyError as e:
        print(f"Error: Clave de configuración faltante en config.yaml: {e}")
        sys.exit(1)

    EXEC_PATH = PROJECT_ROOT / paths_cfg.get('build_dir', 'build') / 'main'
    RESULTS_ROOT = PROJECT_ROOT / paths_cfg.get('results_genetico', 'results/exp_genetico')

    # Parámetros de las instancias (leídos de cfg)
    SEEDS = cfg['seeds']
    K = cfg['k']
    G_SIZE_MIN = cfg['G_size_min']
    F_N_MIN, F_N_MAX = cfg['F_n_min'], cfg['F_n_max']
    F_SIZE_MIN, F_SIZE_MAX = cfg['Fi_size_min'], cfg['Fi_size_max']
    
    # Parámetros del Genético (leídos de cfg)
    timeout_ga = cfg['timeouts']['ga_default_sec']
    ga_args = ['--time_limit', str(timeout_ga)]

    # ------------------------------------------------------------
    # PREPARAR DIRECTORIOS
    # ------------------------------------------------------------
    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
    exp_dir = RESULTS_ROOT / timestamp
    exp_dir.mkdir(parents=True, exist_ok=True)

    path_repro = exp_dir / "dataset.txt"
    path_result = exp_dir / "resultados.txt"

    # ------------------------------------------------------------
    # CABECERA ARCHIVOS (Ahora usa cfg)
    # ------------------------------------------------------------
    fecha = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    header_repro = f"""===============================================================================
DATASET - EXP_GENETICO (Exp. 2)
Fecha: {fecha}
===============================================================================

PARÁMETROS DE CONFIGURACIÓN (desde config.yaml):
  U_SIZE: {U_SIZE}
  G_SIZE_MIN: {G_SIZE_MIN}
  F_N_MIN: {F_N_MIN}
  F_N_MAX: {F_N_MAX}
  F_SIZE_MIN: {F_SIZE_MIN}
  F_SIZE_MAX: {F_SIZE_MAX}
  K: {K}
  TIMEOUT_GA_SEC: {timeout_ga}
  SEMILLAS: {SEEDS}
  ALGORITMOS: genetico (NSGA-II) + greedy

===============================================================================
"""

    with open(path_repro, "w", encoding="utf-8") as f:
        f.write(header_repro)

    with open(path_result, "w", encoding="utf-8") as f:
        f.write("===============================================================================\n")
        f.write("RESULTADOS - EXPERIMENTO EXP_GENETICO (Exp. 2)\n")
        f.write(f"Fecha: {fecha}\n")
        f.write("===============================================================================\n\n")
        
    # ------------------------------------------------------------
    # BUCLE PRINCIPAL DE EXPERIMENTOS
    # ------------------------------------------------------------
    print(f"\nIniciando Experimento 2 (Genetico)...")
    print(f"Total de instancias: {len(SEEDS)}")
    print(f"Límite de tiempo por instancia: {timeout_ga}s")

    for i, seed in enumerate(SEEDS, start=1):
        print(f"\n>>> Ejecutando experimento {i}/{len(SEEDS)} con semilla {seed}...\n")
        
        # El comando ahora se construye con las variables de cfg
        cmd = [
            str(EXEC_PATH), 
            "--no-test", 
            "--seed", str(seed), 
            "--k", str(K), 
            "--G", str(G_SIZE_MIN),
            "--Fmin", str(F_N_MIN), 
            "--Fmax", str(F_N_MAX), 
            "--FsizeMin", str(F_SIZE_MIN), 
            "--FsizeMax", str(F_SIZE_MAX)
        ] + ga_args # <-- Añadimos el time_limit
        
        proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, cwd=PROJECT_ROOT, check=False)
        
        full_output = proc.stdout or ""
        
        match = re.search(rf'{DELIMITER_START}(.*?){DELIMITER_END}', full_output, re.DOTALL)

        if match:
            repro_data = match.group(1).strip() # Datos de conjuntos (G y F)
            fout_data = full_output.replace(match.group(0), "").strip()
        else:
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
            
            # Comando de reproducibilidad (ahora usa cfg)
            frep.write(f"Comando para reproducir:\n")
            frep.write(
                f"  {EXEC_PATH} --no-test "
                f"--seed {seed} "
                f"--k {K} "
                f"--G {G_SIZE_MIN} "
                f"--Fmin {F_N_MIN} --Fmax {F_N_MAX} "
                f"--FsizeMin {F_SIZE_MIN} --FsizeMax {F_SIZE_MAX} "
                f"--time_limit {timeout_ga}\n\n"
            )
            
            if repro_data:
                frep.write(repro_data)
                frep.write("\n")

    print(f"\n✅ Experimento completado.\nResultados guardados en: {exp_dir}\n")

if __name__ == '__main__':
    main()
