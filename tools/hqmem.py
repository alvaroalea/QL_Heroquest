"""Utilidades comunes: memoria del snapshot de Hero Quest 128K por banco."""
import os
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXT = os.path.join(ROOT, 'asset', 'extraido')
BANKS = [open(os.path.join(EXT, 'bank%d.bin' % i), 'rb').read() for i in range(8)]

def byte(addr, bank=0):
    """Byte de la memoria de 64K con 'bank' paginado en 0xC000."""
    if addr < 0x4000: return 0
    if addr < 0x8000: return BANKS[5][addr - 0x4000]
    if addr < 0xC000: return BANKS[2][addr - 0x8000]
    return BANKS[bank][addr - 0xC000]

def load_cov(path=os.path.join(ROOT, 'asset', 'trace', 'cov.txt')):
    cov = set()
    for l in open(path):
        b, a = l.split(); cov.add((int(b), int(a, 16)))
    return cov

OVERLAY = open(os.path.join(EXT, 'overlay_menu_5B00.bin'), 'rb').read() if os.path.exists(os.path.join(EXT, 'overlay_menu_5B00.bin')) else b''
def use_overlay():
    """Hace que byte() lea la imagen del overlay del menu (org 0x5B00) en vez de la memoria fija."""
    global byte
    def byte(addr, bank=0):
        return OVERLAY[addr - 0x5B00] if 0x5B00 <= addr < 0x5B00 + len(OVERLAY) else 0
    globals()['byte'] = byte
