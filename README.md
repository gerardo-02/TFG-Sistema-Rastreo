Proyecto de Fin de Grado realizado en 2025
Se trata de un sistema embebido que rastrea y monitoriza su propia posición en tiempo real.

En el directorio /firmware se encuentra la programación del microcontrolador PIC32MZ2048EFM144.
Los archivos generados automáticamente por MCC se encuentran en los todos directorios excepto en _tests.
Los archivos que no se encuentran dentro de ningún directorio son los creados para este proyecto.

En el directorio /server se encuentran los archivos relativos a la ejecución del servidor que gestiona
la recepción de la posición del microcontrolador. Deben ser situados en el mismo directorio en una máquina
y ejecutados mediante:
python3 server.py
