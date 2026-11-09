#!/bin/sh
# Installed command wrapper. The runtime and standard files live under lib/mint.
MINT_PREFIX='@MINT_PREFIX@'
exec "$MINT_PREFIX/lib/mint/mint" "$@"
