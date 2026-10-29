#!/usr/bin/env python3
"""Extrae las 14 misiones a ficheros de texto editables asset/misiones/misionNN.es.txt (version espanola) o misionNN.en.txt (version inglesa: HQ_VERSION=en) (formato en docs/ZX_DATOS_QL.md).
Con HQ_JUEGO=witchlord extrae las 10 misiones de la ampliacion Return of the Witch Lord (asset/extraido_wl*) a asset/misiones/witchlord_NN.<idioma>.txt y escribe
asset/misiones/witchlord.<idioma>.txt con el subtitulo que sale bajo HEROQUEST en los menus (lo lee tools/hq_misiones_empaquetar.py para el indice).
NO escribe en asset/misiones_ql/ (alli van las misiones retocadas). El mapa tiene 26 columnas x 19 filas; cada casilla es
'AA.BB' (hexadecimal): AA = capa A (bits 0-5 numero de habitacion, bit 6 visible en el plano, bit 7 visible en 3D),
BB = capa B (bit 6 pared oeste, bit 7 pared norte, bit 4 puerta oeste, bit 5 puerta norte, bit 2 puerta abierta, bit 3 puerta secreta o bloqueada).
Uso: [HQ_VERSION=en] hq_misiones_extraer.py [directorio_salida]"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hqmem as m
import hq_missions as hm
import hq_txt_lib as T
ROOT = T.ROOT
D = T.D
IDIOMA = {'es': 'ZX 128K espanol', 'en': 'ZX 128K ingles'}[T.VERSION]
PREFIJO = 'mision%02d' if T.JUEGO == 'hq' else 'witchlord_%02d'
def _objetos_wl():
    """la ampliacion tiene el mismo motor con los datos desplazados: la tabla de indices de nombre de objeto y los 15 nombres se buscan en su memoria
    por el contenido que tienen en Hero Quest"""
    ori = open(os.path.join(ROOT, 'asset', 'extraido' + ('' if T.VERSION == 'es' else '_en'), 'main48.bin'), 'rb').read()
    wl = bytes(m.byte(a) for a in range(0x4000, 0x10000))
    ref = DIR_HQ[T.VERSION]; res = {}
    for k, ln in (('obj_idx', 0x2D), ('objetos', 150)):
        i = wl.find(ori[ref[k] - 0x4000:ref[k] - 0x4000 + ln]); res[k] = 0x4000 + i if i >= 0 else None
    return res
DIR_HQ = T.DIR
EPILOGO = {'es': (0xB9A9, 0xBBD2), 'en': (0xBA19, 0xBC23)}     # las dos paginas del epilogo de la ampliacion (guiones de la memoria fija)
def nombre_objeto(c):
    if 'obj_idx' not in D: D.update(_objetos_wl())
    if not D['obj_idx'] or not D['objetos']: return ''
    idx = m.byte(D['obj_idx'] + c - 0x30) if 0x30 <= c < 0x5D else 0; a = D['objetos'] + 10 * idx
    return ''.join(chr(m.byte(a + k)) for k in range(10) if 0x30 <= m.byte(a + k) < 0x5F).strip().lower()
def nombres_menu():
    """los nombres del menu de misiones (guion del overlay, una linea por fila desde la 4)"""
    seg, _ = T.guion(T.b_ov, D['nombres_menu'], None); lin = [l for l in T.guion_a_lineas(seg) if l[0] == 'texto']
    por_fila = {l[3]: T.decodificar(l[4]).strip(' ') for l in lin}
    return [por_fila[4 + i] for i in range(hm.N_MIS)]
def main(dest):
    os.makedirs(dest, exist_ok=True); nombres = nombres_menu()
    for n in range(hm.N_MIS):
        r = hm.mision(n); A, B = r['capa_a'], r['capa_b']; L = []
        L.append('# Mision %d de %s (original %s: banco %d, 0x%04X) - generado por tools/hq_misiones_extraer.py' % (n, 'Hero Quest' if T.JUEGO == 'hq' else 'Return of the Witch Lord', IDIOMA, r['bank'], r['ptr']))
        L.append('# EDITAR LA COPIA de asset/misiones_ql/%s.%s.txt (este' % (PREFIJO.replace('%02d', 'NN'), T.VERSION) + ' fichero se regenera). Formato y significado de los campos en docs/ZX_DATOS_QL.md.')
        L.append('')
        L.append('mision %d' % n)
        L.append('nombre %s' % nombres[n])
        L.append('# eventos: banderas de comportamiento (numeros de bit; cada bit repite lo especial de esa mision de Hero Quest, ver docs/ZX_MISIONES.md):')
        L.append('#   0 solo el primero que sale cobra 100 | 1 Sir Ragnar | 3 oro de Magnus | 4 roca | 5 sin equipo ni hechizos | 6 cada heroe que sale cobra 100 | 7 sin hechizos de fuego | 9 castillo | 11 sala que se cierra | 12 el monstruo 1 cobra vida al pisar la columna 5 (filas 3 y 4) | 13 teletransporte al salir de la sala de la linea teleporte | 14 la roca de la trampa cae a la derecha del heroe')
        L.append('eventos %s' % (str(n) if T.JUEGO == 'hq' and n in (0, 1, 3, 4, 5, 6, 7, 9, 11) else ('13' if T.JUEGO == 'witchlord' and n == 1 else ('14' if T.JUEGO == 'witchlord' and n == 7 else '-'))))
        L.append('# muerte: rutina al morir un monstruo segun su estadistica (sin linea, la normal): muerte <estadistica> normal | epilogo | salida [casillas a borrar (columna,fila) >] casillas de la escalera (1 a 4, codigos 0x38 a 0x3B)')
        if T.JUEGO == 'witchlord' and n == 4: L.append('muerte 10 salida 13,7 13,8 > 14,7 14,8 15,7 15,8')
        if T.JUEGO == 'witchlord' and n == 9: L.append('muerte 14 epilogo')
        L.append('# errante: ruta de los monstruos con el bit 7 en b6 y sin vida (la niebla de la muerte): errante <pasos por ronda> <tramos que se repiten>, tramo = x o y, + o - y la coordenada con la que acaba')
        if T.JUEGO == 'witchlord' and n == 2: L.append('errante 6 x-0 y-0 x+15 x-0 y+18 x+24')
        L.append('# teleporte: con la bandera 13, salir de la sala indicada lleva a un destino al azar: teleporte <sala> <destinos>, destino = sala,columna,fila,columna de la puerta,fila de la puerta (1 a 6)')
        if T.JUEGO == 'witchlord' and n == 1: L.append('teleporte 31 7,9,9,10,9 7,9,9,10,9 8,13,6,13,7 13,12,12,12,11 13,12,12,12,11 9,16,9,16,9')
        L.append('')
        L.append('# heroes: indice|columna|fila|grafico inicial (se asigna a los 4 heroes en orden: barbaro, enano, elfo, hechicero)')
        for i, h in enumerate(r['heroes']): L.append('heroe %d|%d|%d|%d' % ((i,) + tuple(h)))
        L.append('')
        L.append('[mapa] 26 columnas x 19 filas, cada casilla AA.BB (capa A . capa B) en hexadecimal')
        for y in range(19): L.append(' '.join('%02X.%02X' % (A[y * 26 + x], B[y * 26 + x]) for x in range(26)))
        L.append('')
        L.append('[indicadores] 86 bytes iniciales (se copian a 0x6BB6 del original): marcas de habitaciones, cofres y puertas ya usados')
        fl = list(r['flags'])
        if T.JUEGO == 'witchlord': fl = [v + 18 if 1 <= v <= 11 else v for v in fl]      # salas especiales de la ampliacion: indicadores 19 a 29 (los 1 a 18 son los de Hero Quest, src/mision.c)
        for i in range(0, 86, 26): L.append(' '.join('%02X' % v for v in fl[i:i + 26]))
        L.append('')
        L.append('[monstruos] estado|columna|fila|b3|b4|b5|b6|b7|tipo|estadistica   (10 bytes del original; b3-b5 los pone el juego)')
        for mo in r['monstruos']: L.append('monstruo|' + '|'.join(str(v) for v in mo))
        L.append('')
        L.append('[objetos] columna|fila|codigo   (codigos de la capa de objetos: 0x30-0x5B; el nombre es solo orientativo)')
        for x, y, c in r['objetos']: L.append('objeto|%d|%d|%d ; 0x%02X %s' % (x, y, c, c, nombre_objeto(c)))
        L.append('')
        L.append('[objetivo] texto de presentacion: atributo(hex)|alineacion(L/C/R)|columna|fila|texto')
        tx = r['texto']; seg, _ = T.guion(lambda a: tx[a] if a < len(tx) else 0, 0, len(tx))      # el intérprete del juego se detiene en el primer 00 o FA
        for l in T.guion_a_lineas(seg):
            if l[0] != 'texto': continue
            _, at, x, y, bs = l; t = T.decodificar(bs); lead = len(t) - len(t.lstrip(' ')); core = t.strip(' ')
            if core: x += lead; t = core
            al, col = T.alineacion(x, len(t)); L.append('%02X|%s|%d|%d|%s' % (at, al, col, y, t))
        open(os.path.join(dest, (PREFIJO + '.%s.txt') % (n, T.VERSION)), 'w', encoding='utf-8').write('\n'.join(L) + '\n')
    aventura(dest)
    print('%d misiones en %s' % (hm.N_MIS, dest))
def tipos_monstruo():
    """[monstruos] de la aventura: una linea nombre|retrato|icono|sprite por tipo de monstruo (el tipo 0 primero). Los nombres salen de la memoria del juego; el retrato de combate
    (tabla 0x9ACA, por tipo % 13), la baldosa del plano (0xC3B2, por tipo % 14) y el sprite (el del mismo numero) son los que usaba el motor, que repetia las tablas para los
    tipos que no tenian (la ampliacion usa hasta el 15): se personalizan en la copia de asset/misiones_ql. Los de la ampliacion son los suyos."""
    ret = (11, 4, 5, 6, 7, 10, 8, 4, 4, 4, 7, 7, 7); ico = (20, 21, 22, 23, 24, 25, 26, 27, 27, 27, 27, 27, 27, 27)
    am, nm = D['monstruos']
    if T.JUEGO == 'witchlord':           # los de las tablas de la ampliacion (retratos 0x976F, baldosas 0xBF88 y sprites 0xBE10); la baldosa 33 es la voluta de la niebla (0xAE3F)
        RET, ICO, SPR = (11, 4, 5, 6, 7, 10, 8, 4, 7, 4, 4, 4, 6, 4, 4, 4), (20, 21, 22, 23, 24, 25, 26, 27, 24, 33, 27, 27, 23, 27, 27, 21), (0, 1, 2, 3, 4, 5, 6, 7, 4, 7, 7, 7, 3, 7, 7, 1)
    else: RET, ICO, SPR = None, None, None
    L = ['', '[monstruos] un tipo por linea, el primero es el tipo 0: nombre (hasta 9 caracteres)|retrato de combate|baldosa del plano|sprite de la vista|clase: N normal, V orco (segundo ataque de la vara de orco), M muerto viviente (espada espiritual y agua santa) (los tipos son los de la columna tipo de [monstruos] de las misiones)']
    for t in range(nm):
        nom = T.decodificar(bytes(T.b_main(am + t * 10 + k) for k in range(9))).strip(' ')
        L.append('%s|%d|%d|%d|%s' % (nom, RET[t] if RET else ret[t % 13], ICO[t] if ICO else ico[t % 14], SPR[t] if SPR else t, 'V' if t == 1 else ('M' if t in (4, 5, 6) else 'N')))
    return L
def estadisticas_monstruo():
    """[estadisticas] de la aventura: una linea movimiento|ataque|defensa|monedas|a quien|objeto|mensaje|jefe por estadistica de monstruo (la primera es la 0). Salen de la tabla del juego
    (0xAA8C en Hero Quest, 20 entradas de 5 bytes; 0xA76D en la ampliacion, 16) de la version espanola, que son los mismos en las dos: movimiento, ataque, defensa y el puntero a la
    rutina de muerte, que en Hero Quest da la recompensa (monedas, a quien, bit de objeto y mensaje; las rutinas estan en el empaquetador de la vista) y en la ampliacion es un RET
    (0x96CD) salvo las de las estadisticas 10 y 14, que son las lineas muerte de las misiones. El movimiento no tiene efecto en el original (el DEC A que lo gasta no deja acarreo).
    Jefe (solo la espada espiritual le hace dano): las estadisticas 11 y 12 en Hero Quest (0x7563: INC A, AND $FE, CP $0C) y solo la 14 en la ampliacion (0x74FA: CP $0E)."""
    rec = {0x9A25: (0, 0, 0, 0), 0x7602: (100, 1, 0, 0), 0x7608: (150, 2, 0, 0), 0x761E: (10, 1, 0, 0), 0x7624: (20, 1, 0, 0), 0x7637: (30, 1, 0, 0), 0x762A: (30, 1, 3, 1), 0x763D: (200, 1, 0, 2)}
    wl = T.JUEGO == 'witchlord'
    mem = open(os.path.join(ROOT, 'asset', 'extraido_wl' if wl else 'extraido', 'main48.bin'), 'rb').read()
    base, n = (0xA76D, 16) if wl else (0xAA8C, 20)
    L = ['', '[estadisticas] una por linea, la primera es la estadistica 0 (columna estadistica de [monstruos] de las misiones): movimiento (sin efecto en el original)|dados de ataque|dados de defensa|monedas al morir|a quien (0 nadie, 1 el que lo mata, 2 los cuatro)|bit de objeto (0 ninguno)|mensaje (0 ninguno, 1 BANE DE ORC, 2 ESTRELLA DEL OESTE)|jefe (1 = solo la espada espiritual le hace dano)']
    for i in range(n):
        a = base - 0x4000 + 5 * i; mov, atq, df = mem[a], mem[a + 1], mem[a + 2]; ptr = mem[a + 3] | mem[a + 4] << 8
        r = (0, 0, 0, 0) if wl else rec[ptr]
        L.append('%d|%d|%d|%d|%d|%d|%d|%d' % (mov, atq, df, r[0], r[1], r[2], r[3], (1 if i == 14 else 0) if wl else (1 if i in (11, 12) else 0)))
    return L
def aventura(dest):
    """asset/misiones/<aventura>.<idioma>.txt: datos de la aventura que van en el indice: el subtitulo del menu (segunda linea de texto del titulo; solo la ampliacion), sus tipos
    de monstruo y su epilogo (solo la ampliacion)"""
    if T.JUEGO == 'hq':
        L = ['# Datos de la aventura (Hero Quest, ZX 128K %s) - generado por tools/hq_misiones_extraer.py' % IDIOMA.split(' ', 2)[2],
             '# EDITAR LA COPIA de asset/misiones_ql/heroquest.%s.txt (este fichero se regenera).' % T.VERSION,
             '# subtitulo (opcional): lo que sale bajo HEROQUEST en los menus (maximo 32 caracteres)']
        L += tipos_monstruo() + estadisticas_monstruo()
        open(os.path.join(dest, 'heroquest.%s.txt' % T.VERSION), 'w', encoding='utf-8').write('\n'.join(L) + '\n')
        return
    seg, _ = T.guion(T.b_ov, D['titulo'], None); lin = [l for l in T.guion_a_lineas(seg) if l[0] == 'texto']
    sub = T.decodificar(lin[1][4]).strip(' ')
    L = ['# Datos de la aventura (ampliacion Return of the Witch Lord, %s) - generado por tools/hq_misiones_extraer.py' % IDIOMA,
         '# EDITAR LA COPIA de asset/misiones_ql/witchlord.%s.txt (este fichero se regenera).' % T.VERSION,
         '# subtitulo: lo que sale bajo HEROQUEST en los menus (maximo 32 caracteres); las misiones son witchlord_NN.%s.txt' % T.VERSION,
         'subtitulo %s' % sub]
    L += tipos_monstruo() + estadisticas_monstruo()
    L += ['', '[epilogo] paginas que salen al morir un monstruo con la rutina de muerte epilogo (linea "pagina" y, por pagina, atributo(hex)|alineacion(L/C/R)|columna|fila|texto)']
    for a in EPILOGO[T.VERSION]:
        seg, _ = T.guion(T.b_main, a, a + 0x400); L.append('pagina')
        for l in T.guion_a_lineas(seg):
            if l[0] != 'texto': continue
            _, at, x, y, bs = l; t = T.decodificar(bs); lead = len(t) - len(t.lstrip(' ')); core = t.strip(' ')
            if core: x += lead; t = core
            al, col = T.alineacion(x, len(t)); L.append('%02X|%s|%d|%d|%s' % (at, al, col, y, t))
    open(os.path.join(dest, 'witchlord.%s.txt' % T.VERSION), 'w', encoding='utf-8').write('\n'.join(L) + '\n')
if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'asset/misiones'))
