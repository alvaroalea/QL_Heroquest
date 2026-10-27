#!/bin/bash
# Arranca build/heroquest.img en sQLux (tmp/sQLux/build/sqlux) con Minerva. Uso: tools/ql_run.sh [segundos_maximos]
R=$(cd "$(dirname "$0")/.." && pwd); cd "$R"
exec timeout ${1:-3600} tmp/sQLux/build/sqlux -f tmp/sqlux_hq.ini -w 2x
