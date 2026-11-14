# Battle Mage

> 🚧 **Proyecto en desarrollo.** Este README es un resumen rápido; se ampliará cuando avance el proyecto.

Simulador de combates mágicos en 3D, hecho en C++ con OpenGL 3.3. Dos magos controlados por una IA se enfrentan solos: el combate se simula primero sin gráficos y después se reproduce en una arena 3D.

Proyecto en equipo (6 personas) de la asignatura **Visualització Gràfica Interactiva (VGI)** de la UAB.

## Estado actual

- Simulación del combate a paso fijo y reproducible, separada de los gráficos.
- IA de los magos con máquina de estados: esperar, perseguir, atacar y huir.
- Hechizos con maná, alcance y cooldown, y detección de impactos.
- Arena 3D con iluminación, sombras, texturas y modelos `.obj`.
- Demo de la fase 0 terminada (31/10/2026).

## Ejecutarlo

Windows 10/11 de 64 bits con Visual Studio (carga de trabajo de C++):

1. `git clone https://github.com/CatCodeMage/batleMage.git`
2. Abre `posibleProyecto.slnx`.
3. Elige **x64** y **Debug**, y ejecuta.

Las librerías (GLFW, GLEW, GLM, SOIL2, tinyobjloader, nlohmann/json) van incluidas en el repo. Hay más detalles en [`docs/`](docs/).
