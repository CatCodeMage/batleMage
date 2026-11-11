# server.py — servidor local con soporte para guardar combat.json
import http.server
import socketserver
import json
import os

PORT = 8000
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DB_DIR = os.path.join(BASE_DIR, '..', 'db')
COMBAT_FILE = 'combat_1.json'


class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=BASE_DIR, **kwargs)

    def do_POST(self):
        if self.path == '/save-combat':
            try:
                length = int(self.headers.get('Content-Length', 0))
                body = self.rfile.read(length)
                data = json.loads(body.decode('utf-8'))

                os.makedirs(DB_DIR, exist_ok=True)
                out_path = os.path.join(DB_DIR, COMBAT_FILE)
                with open(out_path, 'w', encoding='utf-8') as f:
                    json.dump(data, f, indent=2, ensure_ascii=False)

                self.send_response(200)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps({"ok": True, "path": out_path}).encode())
                print(f"[save-combat] Guardado en {out_path}")
            except Exception as e:
                self.send_response(500)
                self.send_header('Content-Type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps({"ok": False, "error": str(e)}).encode())
                print(f"[save-combat] ERROR: {e}")
        else:
            self.send_response(404)
            self.end_headers()

    def log_message(self, fmt, *args):
        first = args[0] if args else ''
        if isinstance(first, str) and (
            'favicon' in first or '.well-known' in first
        ):
            return
        super().log_message(fmt, *args)


if __name__ == '__main__':
    with socketserver.TCPServer(("127.0.0.1", PORT), Handler) as httpd:
        print(f"Sirviendo en http://127.0.0.1:{PORT}/")
        print(f"Los guardados irán a: {os.path.abspath(os.path.join(DB_DIR, COMBAT_FILE))}")
        httpd.serve_forever()