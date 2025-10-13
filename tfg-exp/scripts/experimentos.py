#!/usr/bin/env python3
import subprocess
import sys
import os
import yaml
from datetime import datetime
from pathlib import Path
import re

# Obtener directorio raíz del proyecto (TFG-EXP)
SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
TFGCORE_DIR = PROJECT_ROOT / 'tfgcore'

def ocultar_conjuntos_en_texto(texto: str) -> str:
    """
    Quita de la salida las líneas que listan los elementos de la instancia:
    - UNIVERSO_U: ...
    - CONJUNTO_G: ...
    - NUM_CONJUNTOS_F: ...
    - F0: ..., F1: ..., etc.
    Conserva el resto de la salida intacta.
    """
    out = []
    pat_F = re.compile(r'^\s*F\d+\s*:')

    for line in texto.splitlines():
        s = line.strip()
        if s.startswith('=== CONJUNTOS ==='):
        	continue
        if s.startswith('UNIVERSO_U:'):
            continue
        if s.startswith('CONJUNTO_G:'):
            continue
        if s.startswith('NUM_CONJUNTOS_F:'):
            continue
        if pat_F.match(s):
            continue
        out.append(line)
    return '\n'.join(out) + '\n'


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
        src_dir / 'greedy.cpp',
        src_dir / 'genetico.cpp',
        src_dir / 'spea2.cpp'
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

def ejecutar_experimento(programa_path, config, seed, num_ejecucion, total, algo="all"):
    """Ejecuta el programa con una semilla específica"""
    print(f"Ejecutando experimento {num_ejecucion}/{total} con semilla {seed}...")
    
    run_cmd = [
        str(programa_path),
        '--algo', algo,
        '--G', str(config['G_size_min']),
        '--Fmin', str(config['F_n_min']),
        '--Fmax', str(config['F_n_max']),
        '--FsizeMin', str(config['Fi_size_min']),
        '--FsizeMax', str(config['Fi_size_max']),
        '--k', str(config['k']),
        '--seed', str(seed)
    ]
    
    result = subprocess.run(run_cmd, capture_output=True, text=True, cwd=PROJECT_ROOT)
    
    if result.returncode != 0:
        print(f"Error en ejecución con semilla {seed}:")
        print(result.stderr)
        return None
    
    return result.stdout

def parsear_salida(output):
    """Extrae información de la salida del programa con múltiples algoritmos"""
    lines = output.strip().split('\n')
    datos = {
        'semilla': None,
        'u_size': None,
        'g_size': None,
        'f_count': None,
        'k': None,
        'algoritmos': {},
        'conjuntos': {}, 
        'salida_completa': output
    }
    
    current_algo = None
    en_seccion_conjuntos = False
    
    for line in lines:
        # Información de la instancia
        if 'Semilla:' in line:
            datos['semilla'] = line.split(':')[1].strip()
        elif 'U_size:' in line:
            datos['u_size'] = line.split(':')[1].strip()
        elif 'G_size:' in line:
            datos['g_size'] = line.split(':')[1].strip()
        elif 'F_count:' in line:
            datos['f_count'] = line.split(':')[1].strip()
        elif line.strip().startswith('k:'):
            datos['k'] = line.split(':')[1].strip()
        
        # Información por algoritmo (sección SALIDA_ESTRUCTURADA)
        elif 'ALGORITMO:' in line:
            current_algo = line.split(':')[1].strip()
            datos['algoritmos'][current_algo] = {
                'tiempo_ms': None,
                'num_pareto': None,
                'mejor_jaccard': None
            }
        elif 'TIEMPO_MS:' in line and current_algo:
            datos['algoritmos'][current_algo]['tiempo_ms'] = line.split(':')[1].strip()
        elif 'NUM_PARETO:' in line and current_algo:
            datos['algoritmos'][current_algo]['num_pareto'] = line.split(':')[1].strip()
        elif 'MEJOR_JACCARD:' in line and current_algo:
            datos['algoritmos'][current_algo]['mejor_jaccard'] = line.split(':')[1].strip()
        
        # NUEVO: Sección de conjuntos
        elif '=== CONJUNTOS ===' in line:
            en_seccion_conjuntos = True
        elif en_seccion_conjuntos:
            if 'UNIVERSO_U:' in line:
                datos['conjuntos']['U'] = line.split(':')[1].strip()
            elif 'CONJUNTO_G:' in line:
                datos['conjuntos']['G'] = line.split(':')[1].strip()
            elif 'NUM_CONJUNTOS_F:' in line:
                datos['conjuntos']['F_count'] = line.split(':')[1].strip()
            elif line.startswith('F') and ':' in line:
                partes = line.split(':', 1)
                nombre_conjunto = partes[0].strip()
                elementos = partes[1].strip()
                datos['conjuntos'][nombre_conjunto] = elementos
    
    # Si no se encontraron algoritmos en SALIDA_ESTRUCTURADA, buscar formato antiguo
    if not datos['algoritmos']:
        tiempo_ms = None
        num_pareto = None
        for line in lines:
            if 'Tiempo_ejecucion_ms:' in line:
                tiempo_ms = line.split(':')[1].strip()
            elif 'Numero_soluciones_pareto:' in line:
                num_pareto = line.split(':')[1].strip()
        
        if tiempo_ms or num_pareto:
            datos['algoritmos']['unknown'] = {
                'tiempo_ms': tiempo_ms,
                'num_pareto': num_pareto,
                'mejor_jaccard': None
            }
    
    return datos

def crear_directorios(config):
    """Crea los directorios necesarios para los resultados"""
    results_small = PROJECT_ROOT / config.get('results_small', 'results/small')
    results_batch = PROJECT_ROOT / config.get('results_batch', 'results/batch')
    logs = PROJECT_ROOT / config.get('logs','logs')
    
    results_small.mkdir(parents=True, exist_ok=True)
    results_batch.mkdir(parents=True, exist_ok=True)
    logs.mkdir(parents=True, exist_ok=True)
    
    print(f"✓ Directorios creados:")
    print(f"  - {results_small}")
    print(f"  - {results_batch}")
    print(f"  - {logs}")

def ejecutar_experimentos_small(programa_path, config):
    """Ejecuta los experimentos pequeños (exhaustiva + greedy + genetico)"""
    
    cfg = config['small_config']
    paths_cfg = config['paths']
    
    if 'seeds' in cfg:
    	seeds = cfg['seeds']
    else:
    	print("Error: small_config requiere 'seeds'")
    	sys.exit(1)
    inst_count = cfg.get('instances', len(seeds))
    seeds = seeds[:inst_count]
    
    print("\n" + "="*80)
    print("EXPERIMENTOS SMALL (Exhaustiva + Greedy + Genetico)")
    print("="*80 + "\n")
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    results_dir = PROJECT_ROOT / paths_cfg['results_small']
    resultados_file = results_dir / f"{timestamp}_resultados_small.txt"
    reprod_file = results_dir / f"{timestamp}_reproducibilidad_small.txt"
    resumen_csv = results_dir / f"{timestamp}_resumen_small.csv"
    
    with open(resultados_file, 'w', encoding='utf-8') as f_res, \
         open(reprod_file, 'w', encoding='utf-8') as f_rep, \
         open(resumen_csv, 'w', encoding='utf-8') as f_csv:
        
        # Escribir encabezados
        f_res.write("=" * 80 + "\n")
        f_res.write(f"RESULTADOS - EXPERIMENTOS SMALL\n")
        f_res.write(f"Fecha: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f_res.write("=" * 80 + "\n\n")
        
        f_rep.write("=" * 80 + "\n")
        f_rep.write(f"DATOS DE REPRODUCIBILIDAD - SMALL\n")
        f_rep.write(f"Fecha: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f_rep.write("=" * 80 + "\n\n")
        
        # CSV header
        f_csv.write("experimento,semilla,algoritmo,tiempo_ms,num_pareto,mejor_jaccard,u_size,g_size,f_count,k\n")
        
        # Escribir parámetros globales
        f_rep.write("PARÁMETROS DE CONFIGURACIÓN:\n")
        f_rep.write(f"  U_SIZE: {config['U_size']}\n")
        f_rep.write(f"  G_SIZE_MIN: {cfg['G_size_min']}\n")
        f_rep.write(f"  F_N_MIN: {cfg['F_n_min']}\n")
        f_rep.write(f"  F_N_MAX: {cfg['F_n_max']}\n")
        f_rep.write(f"  FI_SIZE_MIN: {cfg['Fi_size_min']}\n")
        f_rep.write(f"  FI_SIZE_MAX: {cfg['Fi_size_max']}\n")
        f_rep.write(f"  K: {cfg['k']}\n")
        f_rep.write(f"  SEMILLAS: {seeds}\n")
        f_rep.write(f"  ALGORITMOS: exhaustiva, greedy, genetico\n\n")
        
        # Ejecutar experimentos
        for i, seed in enumerate(seeds, 1):
            output = ejecutar_experimento(programa_path, cfg, seed, i, len(seeds), algo="all")
            if output is None:
                continue
            
            datos = parsear_salida(output)
            
            # Guardar resultados completos
            f_res.write(f"\n{'='*80}\n")
            f_res.write(f"EXPERIMENTO {i}/{len(seeds)} - SEMILLA: {seed}\n")
            f_res.write(f"{'='*80}\n\n")
            f_res.write(ocultar_conjuntos_en_texto(datos['salida_completa']))
            f_res.write("\n")
            
            # Guardar reproducibilidad
            f_rep.write(f"\n{'='*80}\n")
            f_rep.write(f"EXPERIMENTO {i} - SEMILLA: {seed}\n")
            f_rep.write(f"{'='*80}\n")
            f_rep.write(f"Tamaño universo U: {datos['u_size']}\n")
            f_rep.write(f"Cardinal de G: {datos['g_size']}\n")
            f_rep.write(f"Número de conjuntos en F: {datos['f_count']}\n")
            f_rep.write(f"k: {datos['k']}\n\n")
            
            # Información por algoritmo
            for algo_name, algo_data in datos['algoritmos'].items():
                # CSV: una fila por algoritmo
                f_csv.write(f"{i},{seed},{algo_name},{algo_data['tiempo_ms']},")
                f_csv.write(f"{algo_data['num_pareto']},{algo_data.get('mejor_jaccard', 'N/A')},")
                f_csv.write(f"{datos['u_size']},{datos['g_size']},{datos['f_count']},{datos['k']}\n")
            # NUEVO: Guardar conjuntos
            if datos.get('conjuntos'):
            	f_rep.write("CONJUNTOS DE LA INSTANCIA:\n")
            	f_rep.write(f"  Conjunto G (elementos): {datos['conjuntos'].get('G', 'N/A')}\n")
            	f_rep.write(f"  Número de conjuntos F: {datos['conjuntos'].get('F_count', 'N/A')}\n")
            	f_rep.write(f"  Conjuntos F_i:")
            	for key, value in datos['conjuntos'].items():
            		if key.startswith('F') and key != 'F_count':
            			f_rep.write(f"    {key}: {value}\n")
            		f_rep.write("\n")
				
            f_rep.write(f"Comando para reproducir:\n")
            f_rep.write(f"  ./build/programa --algo all --G {cfg['G_size_min']} ")
            f_rep.write(f"--Fmin {cfg['F_n_min']} --Fmax {cfg['F_n_max']} ")
            f_rep.write(f"--FsizeMin {cfg['Fi_size_min']} --FsizeMax {cfg['Fi_size_max']} ")
            f_rep.write(f"--k {cfg['k']} --seed {seed}\n\n")
            
            # Resumen en consola
            print(f"  ✓ Experimento {i} completado:")
    
    print(f"✓ Experimentos SMALL completados")
    print(f"  - Resultados: {resultados_file}")
    print(f"  - Reproducibilidad: {reprod_file}")
    print(f"  - Resumen CSV: {resumen_csv}\n")

def ejecutar_experimentos_batch(programa_path, config):
    """Ejecuta los experimentos batch (50 instancias - solo greedy)"""
    
    cfg = config['batch_config']
    paths_cfg = config['paths']
    
    if 'seed_start' in cfg:
    	start = cfg['seed_start']
    	count = cfg['instances']
    	seeds = list(range(start, start + count))
    	
    print("\n" + "="*80)
    print("EXPERIMENTOS BATCH (Greedy)")
    print("="*80 + "\n")
    
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    results_dir = PROJECT_ROOT / paths_cfg['paths']['results_batch']
    resultados_file = results_dir / f"{timestamp}_resultados_batch.txt"
    reprod_file = results_dir / f"{timestamp}_reproducibilidad_batch.txt"
    resumen_file = results_dir / f"{timestamp}_resumen_batch.csv"
    
    with open(resultados_file, 'w', encoding='utf-8') as f_res, \
         open(reprod_file, 'w', encoding='utf-8') as f_rep, \
         open(resumen_file, 'w', encoding='utf-8') as f_csv:
        
        # Encabezados
        f_res.write("=" * 80 + "\n")
        f_res.write(f"RESULTADOS - EXPERIMENTOS BATCH (GREEDY)\n")
        f_res.write(f"Fecha: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f_res.write("=" * 80 + "\n\n")
        
        f_rep.write("=" * 80 + "\n")
        f_rep.write(f"DATOS DE REPRODUCIBILIDAD - BATCH\n")
        f_rep.write(f"Fecha: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        f_rep.write("=" * 80 + "\n\n")
        
        # CSV header
        f_csv.write("experimento,semilla,tiempo_ms,num_pareto,mejor_jaccard,u_size,g_size,f_count\n")
        
        # Parámetros efectivos
        f_rep.write("PARÁMETROS (BATCH - efectivos):\n")
        f_rep.write(f"  U_SIZE: {root_cfg['U_size']}\n")
        f_rep.write(f"  G_SIZE_MIN: {cfg['G_size_min']}\n")
        f_rep.write(f"  F_N_MIN: {cfg['F_n_min']}\n")
        f_rep.write(f"  F_N_MAX: {cfg['F_n_max']}\n")
        f_rep.write(f"  FI_SIZE_MIN: {cfg['Fi_size_min']}\n")
        f_rep.write(f"  FI_SIZE_MAX: {cfg['Fi_size_max']}\n")
        f_rep.write(f"  K: {cfg['k']}\n")
        f_rep.write(f"  SEMILLAS: {seeds}\n")
        f_rep.write(f"  ALGORITMOS: greedy\n\n")
        
        # Ejecutar experimentos
        for i, seed in enumerate(seeds, 1):
            output = ejecutar_experimento(programa_path, config, seed, i, len(seeds), algo="greedy")
            if output is None:
                continue
            
            datos = parsear_salida(output)
            
            # Resultados completos
            f_res.write(f"\n{'='*80}\n")
            f_res.write(f"EXPERIMENTO {i}/{len(seeds)} - SEMILLA: {seed}\n")
            f_res.write(f"{'='*80}\n\n")
            f_res.write(ocultar_conjuntos_en_texto(datos['salida_completa']))
            f_res.write("\n")
            
            # Extraer datos del greedy
            greedy_data = datos['algoritmos'].get('greedy', {})
            tiempo_ms = greedy_data.get('tiempo_ms', 'N/A')
            num_pareto = greedy_data.get('num_pareto', 'N/A')
            mejor_jaccard = greedy_data.get('mejor_jaccard', 'N/A')
            
            # Reproducibilidad (más compacto para 50 instancias)
            f_rep.write(f"EXP_{i:03d} | SEED: {seed} | T: {tiempo_ms}ms | ")
            f_rep.write(f"Pareto: {num_pareto} | Jaccard: {mejor_jaccard} | ")
            f_rep.write(f"G: {datos['g_size']} | F: {datos['f_count']}\n")
            
            # CSV
            f_csv.write(f"{i},{seed},{tiempo_ms},{num_pareto},{mejor_jaccard},")
            f_csv.write(f"{datos['u_size']},{datos['g_size']},{datos['f_count']}\n")
            
            print(f"  ✓ [{i:2d}/{len(seeds)}] Semilla {seed}: {tiempo_ms}ms, Pareto: {num_pareto}")
    
    print(f"\n✓ Experimentos BATCH completados")
    print(f"  - Resultados: {resultados_file}")
    print(f"  - Reproducibilidad: {reprod_file}")
    print(f"  - Resumen CSV: {resumen_file}\n")

def main():
    print("="*80)
    print("SISTEMA DE EXPERIMENTACIÓN")
    print(f"Directorio del proyecto: {PROJECT_ROOT}")
    print("="*80 + "\n")
    
    # Cargar configuración
    config = cargar_configuracion()
    crear_directorios(config['paths'])
    
    # Compilar
    programa_path = compilar_programa(config['U_size'])
    
    # Ejecutar experimentos
    print("\n¿Qué experimentos deseas ejecutar?")
    print("1. Solo SMALL (4 instancias - exhaustiva + greedy + genético)")
    print("2. Solo BATCH (50 instancias - greedy + genético)")
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
