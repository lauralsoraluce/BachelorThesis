#!/usr/bin/env python3
import subprocess
import sys
import os
import yaml
from datetime import datetime
from pathlib import Path

# Obtener directorio raíz del proyecto (TFG-EXP)
SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
TFGCORE_DIR = PROJECT_ROOT / 'tfgcore'

def ejecutar_experimento_algo(programa_path, config, seed, algo):
    run_cmd = [
        str(programa_path),
        'G', str(config['G_size_min']),
        '--Fmin', str(config['F_n_min']),
        '--Fmax', str(config['F_n_max']),
        '--FsizeMin', str(config['Fi_size_min']),
        '--FsizeMax', str(config['Fi_size_max']),
        '--k', str(config.get('k', 3)),
        '--seed', str(seed),
        '--algo', algo
    ]
    return subprocess.run(run_cmd, capture_output=True, text=True, cwd=PROJECT_ROOT)

def cargar_configuracion():
    """Carga la configuración desde el archivo YAML en la raíz"""
    config_path = PROJECT_ROOT / 'config.yaml'
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

def compilar_programa(u_size):
    """Compila el programa con el tamaño de U especificado"""
    print(f"\nCompilando con U_SIZE={u_size}...")
    
    # Directorios
    src_dir = TFGCORE_DIR / 'src'
    include_dir = TFGCORE_DIR / 'include'
    build_dir = PROJECT_ROOT / 'build'
    
    # Verificar que existe tfgcore
    if not TFGCORE_DIR.exists():
        print(f"Error: No se encuentra el directorio {TFGCORE_DIR}")
        sys.exit(1)
    
    # Crear directorio build si no existe
    build_dir.mkdir(exist_ok=True)
    
    # Archivos fuente
    sources = [
        src_dir / 'main.cpp',
        src_dir / 'exhaustiva.cpp',
        src_dir / 'metrics.cpp',
        src_dir / 'greedy.cpp'
    ]
    
    # Verificar que existen los archivos
    for src in sources:
        if not src.exists():
            print(f"Error: No se encuentra {src}")
            sys.exit(1)
    
    # Comando de compilación
    compile_cmd = [
        'g++',
        f'-DU_SIZE={u_size}',
        '-std=c++17',
        '-O3',
        f'-I{include_dir}',
        '-o', str(build_dir / 'programa')
    ] + [str(s) for s in sources]
    
    # Compilar
    result = subprocess.run(compile_cmd, capture_output=True, text=True, cwd=PROJECT_ROOT)
    
    if result.returncode != 0:
        print("Error en compilación:")
        print(result.stderr)
        sys.exit(1)
    
    print("✓ Compilación exitosa!")
    return build_dir / 'programa'

def ejecutar_experimento(programa_path, config, seed, num_ejecucion, total):
    """Ejecuta el programa con una semilla específica"""
    print(f"Ejecutando experimento {num_ejecucion}/{total} con semilla {seed}...")
    
    run_cmd = [
        str(programa_path),
        'G', str(config['G_size_min']),
        '--Fmin', str(config['F_n_min']),
        '--Fmax', str(config['F_n_max']),
        '--FsizeMin', str(config['Fi_size_min']),
        '--FsizeMax', str(config['Fi_size_max']),
        '--k', str(config.get('k', 3)),
        '--seed', str(seed)
    ]
    
    result = subprocess.run(run_cmd, capture_output=True, text=True, cwd=PROJECT_ROOT)
    
    if result.returncode != 0:
        print(f"Error en ejecución con semilla {seed}:")
        print(result.stderr)
        return None
    
    return result.stdout

def parsear_salida(output):
    """Extrae información de la salida del programa"""
    lines = output.strip().split('\n')
    datos = {
        'u_size': None,
        'g_size': None,
        'f_count': None,
        'tiempo_ms': None,
        'num_pareto': None,
        'salida_completa': output
    }
    
    for line in lines:
        if 'Tamaño universo U:' in line:
            datos['u_size'] = line.split(':')[1].strip()
        elif 'Tamaño conjunto G:' in line:
            datos['g_size'] = line.split(':')[1].strip()
        elif 'Número de conjuntos en F:' in line:
            datos['f_count'] = line.split(':')[1].strip()
        elif 'Tiempo_ejecucion_ms:' in line:
            datos['tiempo_ms'] = line.split(':')[1].strip()
        elif 'Numero_soluciones_pareto:' in line:
            datos['num_pareto'] = line.split(':')[1].strip()
    
    return datos

def crear_directorios(config):
    """Crea los directorios necesarios para los resultados"""
    results_small = PROJECT_ROOT / config['paths']['results_small']
    results_batch = PROJECT_ROOT / config['paths']['results_batch']
    logs = PROJECT_ROOT / config['paths']['logs']
    
    results_small.mkdir(parents=True, exist_ok=True)
    results_batch.mkdir(parents=True, exist_ok=True)
    logs.mkdir(parents=True, exist_ok=True)
    
    print(f"✓ Directorios creados:")
    print(f"  - {results_small}")
    print(f"  - {results_batch}")
    print(f"  - {logs}")

def ejecutar_experimentos_small(programa_path, config):
    """Ejecuta los experimentos pequeños (exhaustiva)"""
    print("\n" + "="*80)
    print("EXPERIMENTOS SMALL (Búsqueda Exhaustiva)")
    print("="*80 + "\n")
    
    seeds = config['seeds_small']
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    
    results_dir = PROJECT_ROOT / config['paths']['results_small']
    resultados_file = results_dir / f"{timestamp}_resultados_exhaustiva.txt"
    reprod_file = results_dir / f"{timestamp}_reproducibilidad_exhaustiva.txt"
    
    with open(resultados_file, 'w', encoding='utf-8') as f_res, \
         open(reprod_file, 'w', encoding='utf-8') as f_rep:
        
        # Escribir encabezados
        f_res.write("=" * 80 + "\n")
        f_res.write(f"RESULTADOS - BÚSQUEDA EXHAUSTIVA\n")
        f_res.write(f"Fecha: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f_res.write("=" * 80 + "\n\n")
        
        f_rep.write("=" * 80 + "\n")
        f_rep.write(f"DATOS DE REPRODUCIBILIDAD - EXHAUSTIVA\n")
        f_rep.write(f"Fecha: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f_rep.write("=" * 80 + "\n\n")
        
        # Escribir parámetros globales
        f_rep.write("PARÁMETROS DE CONFIGURACIÓN:\n")
        f_rep.write(f"  U_SIZE: {config['U_size']}\n")
        f_rep.write(f"  G_SIZE_MIN: {config['G_size_min']}\n")
        f_rep.write(f"  F_N_MIN: {config['F_n_min']}\n")
        f_rep.write(f"  F_N_MAX: {config['F_n_max']}\n")
        f_rep.write(f"  FI_SIZE_MIN: {config['Fi_size_min']}\n")
        f_rep.write(f"  FI_SIZE_MAX: {config['Fi_size_max']}\n")
        f_rep.write(f"  K: {config.get('k', 3)}\n")
        f_rep.write(f"  SEMILLAS: {seeds}\n")
        f_rep.write(f"  TIMEOUT: {config['timeouts']['exhaustive_sec']}s\n\n")
        
        # Ejecutar experimentos
        for i, seed in enumerate(seeds, 1):
            out_exh = ejecutar_experimento_algo(programa_path, config, seed, "exhaustiva")
            if out_exh.returncode != 0:
                print("Error EXH:", out_exh.stderr); continue
            datos_exh = parsear_salida(out_exh.stdout)
    		
            out_gr = ejecutar_experimento_algo(programa_path, config, seed, "greedy")
            if out_gr.returncode != 0:
            	print("Error GR:", out_gr.stderr); continue
            datos_gr = parsear_salida(out_gr.stdout)
            
            # Guardar resultados
            f_res.write(f"\n{'='*80}\n")
            f_res.write(f"EXPERIMENTO {i}/{len(seeds)} - SEMILLA: {seed}\n")
            f_res.write(f"{'='*80}\n\n")
            f_res.write(datos['salida_completa'])
            f_res.write("\n")
            
            # Guardar reproducibilidad
            f_rep.write(f"\n{'='*80}\n")
            f_rep.write(f"EXPERIMENTO {i} - SEMILLA: {seed}\n")
            f_rep.write(f"{'='*80}\n")
            f_rep.write(f"Tamaño universo U: {datos['u_size']}\n")
            f_rep.write(f"Cardinal de G: {datos['g_size']}\n")
            f_rep.write(f"Número de conjuntos en F: {datos['f_count']}\n")
            f_rep.write(f"Tiempo de ejecución: {datos['tiempo_ms']} ms\n")
            f_rep.write(f"Soluciones Pareto: {datos['num_pareto']}\n")
            f_rep.write(f"\nComando para reproducir:\n")
            f_rep.write(f"  ./build/programa G {config['G_size_min']} ")
            f_rep.write(f"--Fmin {config['F_n_min']} --Fmax {config['F_n_max']} ")
            f_rep.write(f"--FsizeMin {config['Fi_size_min']} --FsizeMax {config['Fi_size_max']} ")
            f_rep.write(f"--k {config.get('k', 3)} --seed {seed}\n\n")
            
            print(f"  ✓ Completado (Tiempo: {datos['tiempo_ms']} ms, Pareto: {datos['num_pareto']})\n")
    
    print(f"✓ Experimentos SMALL completados")
    print(f"  - Resultados: {resultados_file}")
    print(f"  - Reproducibilidad: {reprod_file}\n")

def ejecutar_experimentos_batch(programa_path, config):
    """Ejecuta los experimentos batch (50 instancias)"""
    print("\n" + "="*80)
    print("EXPERIMENTOS BATCH (50 instancias)")
    print("="*80 + "\n")
    
    seed_start = config['seed_batch_start']
    num_instances = config['batch_instances']
    seeds = list(range(seed_start, seed_start + num_instances))
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    
    results_dir = PROJECT_ROOT / config['paths']['results_batch']
    resultados_file = results_dir / f"{timestamp}_resultados_batch.txt"
    reprod_file = results_dir / f"{timestamp}_reproducibilidad_batch.txt"
    resumen_file = results_dir / f"{timestamp}_resumen_batch.csv"
    
    with open(resultados_file, 'w', encoding='utf-8') as f_res, \
         open(reprod_file, 'w', encoding='utf-8') as f_rep, \
         open(resumen_file, 'w', encoding='utf-8') as f_csv:
        
        # Encabezados
        f_res.write("=" * 80 + "\n")
        f_res.write(f"RESULTADOS - EXPERIMENTOS BATCH\n")
        f_res.write(f"Fecha: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f_res.write("=" * 80 + "\n\n")
        
        f_rep.write("=" * 80 + "\n")
        f_rep.write(f"DATOS DE REPRODUCIBILIDAD - BATCH\n")
        f_rep.write(f"Fecha: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f_rep.write("=" * 80 + "\n\n")
        
        # CSV header
        f_csv.write("experimento,semilla,tiempo_ms,num_pareto,u_size,g_size,f_count\n")
        
        # Ejecutar experimentos
        for i, seed in enumerate(seeds, 1):
            output = ejecutar_experimento(programa_path, config, seed, i, len(seeds))
            if output is None:
                continue
            
            datos = parsear_salida(output)
            
            # Resultados completos
            f_res.write(f"\n{'='*80}\n")
            f_res.write(f"EXPERIMENTO {i}/{len(seeds)} - SEMILLA: {seed}\n")
            f_res.write(f"{'='*80}\n\n")
            f_res.write(datos['salida_completa'])
            f_res.write("\n")
            
            # Reproducibilidad (más compacto para 50 instancias)
            f_rep.write(f"EXP_{i:03d} | SEED: {seed} | T: {datos['tiempo_ms']}ms | ")
            f_rep.write(f"Pareto: {datos['num_pareto']} | G: {datos['g_size']} | F: {datos['f_count']}\n")
            
            # CSV
            f_csv.write(f"{i},{seed},{datos['tiempo_ms']},{datos['num_pareto']},")
            f_csv.write(f"{datos['u_size']},{datos['g_size']},{datos['f_count']}\n")
            
            print(f"  ✓ [{i:2d}/{len(seeds)}] Semilla {seed}: {datos['tiempo_ms']}ms, Pareto: {datos['num_pareto']}")
    
    print(f"\n✓ Experimentos BATCH completados")
    print(f"  - Resultados: {resultados_file}")
    print(f"  - Reproducibilidad: {reprod_file}")
    print(f"  - Resumen CSV: {resumen_file}\n")

def main():
    print("="*80)
    print("SISTEMA DE EXPERIMENTACIÓN - BÚSQUEDA EXHAUSTIVA")
    print(f"Directorio del proyecto: {PROJECT_ROOT}")
    print("="*80 + "\n")
    
    # Cargar configuración
    config = cargar_configuracion()
    
    # Agregar k si no está en config (valor por defecto)
    if 'k' not in config:
        config['k'] = 3
    
    # Crear directorios
    crear_directorios(config)
    
    # Compilar
    programa_path = compilar_programa(config['U_size'])
    
    # Ejecutar experimentos
    print("\n¿Qué experimentos deseas ejecutar?")
    print("1. Solo SMALL (4 instancias)")
    print("2. Solo BATCH (50 instancias)")
    print("3. Ambos (SMALL + BATCH)")
    
    opcion = input("\nSelecciona una opción (1-3): ").strip()
    
    if opcion == '1':
        ejecutar_experimentos_small(programa_path, config)
    elif opcion == '2':
        ejecutar_experimentos_batch(programa_path, config)
    elif opcion == '3':
        ejecutar_experimentos_small(programa_path, config)
        ejecutar_experimentos_batch(programa_path, config)
    else:
        print("Opción no válida")
        sys.exit(1)
    
    print("\n" + "="*80)
    print("✓ TODOS LOS EXPERIMENTOS COMPLETADOS")
    print("="*80)

if __name__ == '__main__':
    main()
