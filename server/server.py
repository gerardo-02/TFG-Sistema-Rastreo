from flask import Flask, request, jsonify, send_from_directory
app = Flask (__name__)

# Posición GPS almacenada
posicion_actual = {"lat": 0.0, "lon": 0.0, "vel": 0.0, "dir": 0.0}

@app.route(’/posicion’, methods=[’POST’])
def recibir posicion():
	data = request.get_json()

	if data is None: 
		return "JSON no válido o faltante", 400

	try: 
lat = float (data.get(’lat’))
lon = float (data.get(’lon’))
dir = int (data.get(’dir’))
vel = float (data.get(’vel’))

global posicion_actual
posicion_actual = {"lat": lat,"lon": lon, "vel": vel, "dir": dir}

	return "OK", 200
except Exception as e: 
	return f "Error en los datos: {e}", 400

@app.route(’/posicion’, methods=[’GET’])
def enviar posicion(): 
	print("Posición actual enviada:", posicion_actual)
	try: 
		return jsonify (posicion_actual)
	except Exception: 
		return jsonify ({"error": "sin datos"}) , 404

@app.route(’/’)
def index():
	return send_from_directory(’.’, ’index.html’)

if __name__ == ’__main__’:
app.run(host=’0.0.0.0’, port=5000)