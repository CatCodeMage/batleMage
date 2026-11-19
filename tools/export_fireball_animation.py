"""blender -b source.blend --python-exit-code 1 --python tools/export_fireball_animation.py -- OUTPUT_DIR [--frames 15]"""

import argparse
import sys
from pathlib import Path

import bpy
from mathutils import Vector


# Blender is Z-up. BattleMage is Y-up and considers +Z its forward direction.
def game_axes(vector):
    return vector.x, vector.z, -vector.y


def export_obj(path, objects, origin, scale, depsgraph):
    positions = []
    normals = {}
    faces = []
    for source in objects:
        obj = source.evaluated_get(depsgraph)
        mesh = obj.to_mesh()
        try:
            offset = len(positions)
            matrix = obj.matrix_world
            normal_matrix = matrix.to_3x3().inverted().transposed()
            positions.extend(game_axes((matrix @ v.co - origin) * scale) for v in mesh.vertices)
            mesh.calc_loop_triangles()
            for triangle in mesh.loop_triangles:
                corners = []
                for index in triangle.loops:
                    vertex = mesh.loops[index].vertex_index
                    normal = (normal_matrix @ mesh.corner_normals[index].vector).normalized()
                    key = tuple(round(component, 4) for component in game_axes(normal))
                    normal_id = normals.setdefault(key, len(normals) + 1)
                    corners.append(f"{offset + vertex + 1}//{normal_id}")
                faces.append("f " + " ".join(corners))
        finally:
            obj.to_mesh_clear()

    with path.open("w", encoding="ascii", newline="\n") as output:
        output.write("mtllib fireball.mtl\no Fireball\n")
        output.writelines("v %.5f %.5f %.5f\n" % position for position in positions)
        output.writelines("vn %.4f %.4f %.4f\n" % normal for normal in normals)
        output.write("usemtl fire\n")
        output.write("\n".join(faces) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--frames", type=int, default=15)
    args = parser.parse_args(sys.argv[sys.argv.index("--") + 1:])
    if not 1 <= args.frames <= 100:
        parser.error("--frames must be between 1 and 100")
    scene = bpy.context.scene
    objects = sorted(
        (obj for obj in scene.objects if obj.type == "MESH" and not obj.hide_render
         and any(slot.material and slot.material.name == "fire" for slot in obj.material_slots)),
        key=lambda obj: obj.name,
    )
    if not objects:
        raise ValueError("No visible meshes with the source material 'fire'")

    # Keep this reference pivot and scale fixed in every exported frame.
    scene.frame_set(scene.frame_start)
    depsgraph = bpy.context.evaluated_depsgraph_get()
    vertices = []
    for source in objects:
        obj = source.evaluated_get(depsgraph)
        vertices.extend(obj.matrix_world @ v.co for v in obj.data.vertices)
    origin = sum(vertices, Vector()) / len(vertices)
    scale = 1.4 / max((vertex - origin).length for vertex in vertices)

    args.output_dir.mkdir(parents=True, exist_ok=True)
    # Match the orange base. Warm the original yellow-green emission and reduce
    # its strength for the game's shader, which writes directly to the framebuffer.
    (args.output_dir / "fireball.mtl").write_text(
        "newmtl fire\nKd 1.0 0.268 0.0\nKe 0.53 0.15 0.015\n",
        encoding="ascii", newline="\n",
    )
    # The source returns to its first pose at frame_end. Sample [start, end).
    span = scene.frame_end - scene.frame_start
    for index in range(args.frames):
        sample = scene.frame_start + span * index / args.frames
        scene.frame_set(int(sample), subframe=sample - int(sample))
        path = args.output_dir / f"fireball_{index:02d}.obj"
        export_obj(path, objects, origin, scale, depsgraph)
        print(f"{path.name}: Blender frame {sample:g}", flush=True)


if __name__ == "__main__":
    main()
