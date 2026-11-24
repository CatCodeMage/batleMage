"""blender -b source.blend --python-exit-code 1 --python tools/export_fireball_animation.py -- OUTPUT_DIR [--frames 20]"""

import argparse
import sys
from pathlib import Path

import bpy
from mathutils import Vector


PALETTE = (
    ("fire_core", 0.21, (1.0, 0.48, 0.04), (1.0, 0.66, 0.11)),
    ("fire_body", 0.43, (1.0, 0.28, 0.01), (0.88, 0.33, 0.028)),
    ("fire_tips", float("inf"), (1.0, 0.14, 0.0), (0.70, 0.14, 0.012)),
)


# Blender is Z-up. BattleMage is Y-up and considers +Z its forward direction.
def game_axes(vector):
    return vector.x, vector.z, -vector.y


def evaluated_meshes(objects):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    for source in objects:
        obj = source.evaluated_get(depsgraph)
        mesh = obj.to_mesh()
        try:
            yield obj.matrix_world, mesh
        finally:
            obj.to_mesh_clear()


def export_obj(path, objects, origin, scale):
    positions = []
    normals = {}
    faces = {name: [] for name, _, _, _ in PALETTE}
    for matrix, mesh in evaluated_meshes(objects):
        offset = len(positions)
        basis = matrix.to_3x3()
        normal_matrix = basis.inverted().transposed()
        reverse = basis.determinant() < 0
        vertices = [(matrix @ v.co - origin) * scale for v in mesh.vertices]
        positions.extend(map(game_axes, vertices))
        mesh.calc_loop_triangles()
        for triangle in mesh.loop_triangles:
            a, b, c = triangle.vertices
            radius = ((vertices[a] + vertices[b] + vertices[c]) / 3).length
            material = next(name for name, limit, _, _ in PALETTE if radius <= limit)
            corners = []
            loops = reversed(tuple(triangle.loops)) if reverse else triangle.loops
            for index in loops:
                vertex = mesh.loops[index].vertex_index
                normal = (normal_matrix @ mesh.corner_normals[index].vector).normalized()
                key = tuple(round(component, 4) for component in game_axes(normal))
                normal_id = normals.setdefault(key, len(normals) + 1)
                corners.append(f"{offset + vertex + 1}//{normal_id}")
            faces[material].append("f " + " ".join(corners))

    with path.open("w", encoding="ascii", newline="\n") as output:
        output.write("mtllib fireball.mtl\no Fireball\n")
        output.writelines("v %.5f %.5f %.5f\n" % position for position in positions)
        output.writelines("vn %.4f %.4f %.4f\n" % normal for normal in normals)
        for material, group in faces.items():
            if group:
                output.write(f"usemtl {material}\n")
                output.write("\n".join(group) + "\n")

    return positions


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--frames", type=int, default=20)
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
    vertices = [matrix @ v.co for matrix, mesh in evaluated_meshes(objects) for v in mesh.vertices]
    origin = sum(vertices, Vector()) / len(vertices)
    scale = 1.4 / max((vertex - origin).length for vertex in vertices)

    args.output_dir.mkdir(parents=True, exist_ok=True)
    # Use the engine's existing additive Ke. The hottest faces stay bright even in shadow.
    (args.output_dir / "fireball.mtl").write_text(
        "\n".join(
            "newmtl %s\nKd %.3f %.3f %.3f\nKe %.3f %.3f %.3f\n" % (name, *diffuse, *emission)
            for name, _, diffuse, emission in PALETTE
        ),
        encoding="ascii", newline="\n",
    )
    # The source returns to its first pose at frame_end. Sample [start, end).
    span = scene.frame_end - scene.frame_start
    for index in range(args.frames):
        sample = scene.frame_start + span * index / args.frames
        scene.frame_set(int(sample), subframe=sample - int(sample))
        path = args.output_dir / f"fireball_{index:02d}.obj"
        positions = export_obj(path, objects, origin, scale)
        bounds = " ".join(f"[{min(axis):.4f}, {max(axis):.4f}]" for axis in zip(*positions))
        print(f"{path.name}: Blender frame {sample:g}, Y-up bounds {bounds}", flush=True)


if __name__ == "__main__":
    main()
