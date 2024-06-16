mkdir -p libraries
mkdir -p output
mkdir -p output/vga
mkdir -p output/fpdram

vlib libraries/work
vmap work libraries/work

vlib libraries/flex10ke
vmap flex10ke libraries/flex10ke
