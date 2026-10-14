# BattleMage

Proyecto VGI en C++20 y OpenGL 3.3. La demo grafica incluye suelo, cubo animado,
dos luces puntuales y sombras de la luz que orbita. F2 activa/desactiva las sombras.

Abrir `posibleProyecto.slnx` con Visual Studio y elegir **Debug | x64**.
Las librerias estan incluidas en `posibleProyecto/libs`.

- [Instalacion](docs/instalacion.html)
- [Guia tecnica y objetivos](docs/tecnica.html)
- [Sombras: funcionamiento, configuracion, rendimiento y pruebas](docs/sombras.html)

Pruebas desde PowerShell en la raiz del repositorio (Visual C++ y OpenGL 3.3):

```powershell
.\tests\run-tests.ps1
```

Para ejecutar solo las comprobaciones matematicas: `.\tests\run-tests.ps1 -MathOnly`.
