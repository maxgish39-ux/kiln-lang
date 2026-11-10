#!/bin/sh
# Installed command wrapper. The runtime and standard files live under lib/kiln.
KILN_PREFIX='@KILN_PREFIX@'
exec "$KILN_PREFIX/lib/kiln/kiln" "$@"
