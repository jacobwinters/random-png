#!/bin/bash
git diff --no-index --histogram pngrutil.original.c pngrutil.c > pngrutil.patch
git diff --no-index --histogram puff.original.c puff.c > puff.patch
emcc randompng.c pngrutil.c puff.c -O1 -sENVIRONMENT=web -sEXPORTED_FUNCTIONS=_malloc,_free,_fillrandbuffer,_copyrgbtorgba,_copygrayscaletorgba,_fillinflatebuffer,_fillfilterbuffer,_pngfilterdecode
