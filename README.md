Utilizar la file Makefile para los archivos

Los programas requieren ser abiertos en este orden:

1. El servidor central con ./CentralServer
2. La mempool con ./mempool
3. Las mining pool que se consideren necesarias con ./MPserver portno MPaddress

	portno preferentemente 9000,9001,9002, ...,etc.
	MPaddress algun numero entre 1 y 255.
	
4. Los mineros ./Minero portno

	portno debe ser el mismo que la Mining pool a la que quieras conectar este minero.
	Luego pedira introducir una ID cualquier numero entre 1 y 255 menos las MPaddress.
	
5. La wallet sirve para ver cuantas monedas tiene acumulado cada ID
	puede utilizarse con las Mining pools y con los mineros.
