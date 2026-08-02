#!/bin/bash
CYAN='\033[1;36m'
DIM='\033[2m'
NC='\033[0m'

echo ""
echo -e " ${CYAN}CAX SSG${NC} - C STATIC SITE GENERATOR"
echo -e " ${DIM}BY AXCORA TECHNOLOGY${NC}"
echo " ------------------------------------------"
echo ""

if [ "$1" = "" ]; then
  echo " Usage:"
  echo "./cax.sh init [name] - Init new project"
  echo "./cax.sh build - Build site"
  echo "./cax.sh start - Start server"
  echo "./cax.sh serve --watch - Dev mode"
  echo ""
  exit 0
fi

./cax "$@"