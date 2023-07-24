#!/bin/bash

arquivos=("compressor.log" "compilador.log" "matriz.log" "simulador.log")

for arquivo in "${arquivos[@]}"; do
  # Remove the extension from the filename
  nomeArquivo="${arquivo%.*}"

  # Output file path
  arquivoSaida="testes/${nomeArquivo}PagCresc.csv"

  # Check if the output file already exists
  if [ -e "$arquivoSaida" ]; then
      rm "$arquivoSaida"
  fi

  # Define the values as an array
  tamPag=(2 4 8 16 32 64)
  memsFisica=(128 256 512 1024 2048 4096 8192 16384)
  algoritmos=("fifo" "random" "2a" "lru")

  echo "Algoritmo,Tamanho memoria,Tamanho pagina,Total de páginas,Page Faults,Paginas Escritas,Hits" >> "${arquivoSaida}"

  # Loop through the values
  for alg in "${algoritmos[@]}"; do
    for mem in "${memsFisica[@]}"; do
      for pag in "${tamPag[@]}"; do 
          # Run the command with the current value and inputs
          ./tp2virtual "$alg" "$arquivo" "$pag" "$mem" >> "${arquivoSaida}"
          echo >> "${arquivoSaida}"
        done
    done
  done
done