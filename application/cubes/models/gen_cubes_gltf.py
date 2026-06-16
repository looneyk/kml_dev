import json
import struct
import base64
import os

def parse_obj_grouped(filepath):
    positions = []
    texcoords = []
    normals = []
    groups = {}       # ordered: group_name -> list of (v_idx, t_idx, n_idx)
    current_group = 'default'

    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith('v '):
                p = line.split()
                positions.append((float(p[1]), float(p[2]), float(p[3])))
            elif line.startswith('vt '):
                p = line.split()
                texcoords.append((float(p[1]), float(p[2])))
            elif line.startswith('vn '):
                p = line.split()
                normals.append((float(p[1]), float(p[2]), float(p[3])))
            elif line.startswith('g '):
                current_group = line.split()[1]
                if current_group not in groups:
                    groups[current_group] = []
            elif line.startswith('f '):
                parts = line.split()[1:]
                face = []
                for part in parts:
                    idx = part.split('/')
                    v = int(idx[0]) - 1
                    t = int(idx[1]) - 1 if len(idx) > 1 and idx[1] else -1
                    n = int(idx[2]) - 1 if len(idx) > 2 and idx[2] else -1
                    face.append((v, t, n))
                if current_group not in groups:
                    groups[current_group] = []
                for i in range(1, len(face) - 1):
                    groups[current_group].extend([face[0], face[i], face[i + 1]])

    return positions, texcoords, normals, groups

# Cube face brightness matching Scene.cpp set_vertex_colors pattern:
#   colors[] = {color1, color4, color2, color1, color4, color2}
#   face order: -Z, -Y, -X, +Z, +Y, +X  =>  bright, dim, mid, bright, dim, mid
# OBJ group name -> brightness bucket
FACE_BRIGHTNESS = {
    'bottom': 'bright',   # -Z, multiplier 1.0
    'top':    'bright',   # +Z, multiplier 1.0
    'right':  'mid',      # +X, multiplier 0.5
    'left':   'mid',      # -X, multiplier 0.5
    'back':   'dim',      # -Y, multiplier 0.25
    'front':  'dim',      # +Y, multiplier 0.25
}

BRIGHTNESS_INDEX = {'bright': 0, 'mid': 1, 'dim': 2}

def build_cube_mesh(positions, texcoords, normals, groups):
    """Unified vertex list + one index list per face group (preserving OBJ group order)."""
    vertex_map = {}
    out_pos = []
    out_tex = []
    out_norm = []
    face_indices = {}   # group_name -> [idx, ...]

    for group_name, flat_verts in groups.items():
        indices = []
        for (vi, ti, ni) in flat_verts:
            key = (vi, ti, ni)
            if key not in vertex_map:
                vertex_map[key] = len(out_pos)
                out_pos.append(sw_t(*positions[vi]))
                out_tex.append(texcoords[ti] if 0 <= ti < len(texcoords) else (0.0, 0.0))
                out_norm.append(sw_t(*normals[ni]) if 0 <= ni < len(normals) else (0.0, 0.0, 1.0))
            indices.append(vertex_map[key])
        face_indices[group_name] = indices

    return out_pos, out_tex, out_norm, face_indices

def pack_vec3(data):
    buf = b''
    for v in data:
        buf += struct.pack('<fff', v[0], v[1], v[2])
    return buf

def pack_vec2(data):
    buf = b''
    for v in data:
        buf += struct.pack('<ff', v[0], v[1])
    return buf

def pack_u16(data):
    buf = b''
    for i in data:
        buf += struct.pack('<H', i)
    return buf

def pad4(data):
    r = len(data) % 4
    return data + b'\x00' * ((4 - r) % 4)

def sw_t(x, y, z): return [x, z, -y]

def bounds(positions):
    xs = [p[0] for p in positions]
    ys = [p[1] for p in positions]
    zs = [p[2] for p in positions]
    return [min(xs), min(ys), min(zs)], [max(xs), max(ys), max(zs)]

def build_cube_buffer(pos, tex, norm, face_groups, face_indices):
    """Pack vertex data then one index section per face group (in group order)."""
    parts = [pack_vec3(pos), pack_vec2(tex), pack_vec3(norm)]
    parts += [pack_u16(face_indices[g]) for g in face_groups]
    offsets = []
    lengths = []
    data = b''
    for part in parts:
        offsets.append(len(data))
        lengths.append(len(part))
        data += pad4(part)
    return data, offsets, lengths

def make_material(name, r, g, b):
    return {
        "name": name,
        "emissiveFactor": [r, g, b],
        "pbrMetallicRoughness": {
            "baseColorFactor": [0.0, 0.0, 0.0, 1.0],
            "metallicFactor": 0.0,
            "roughnessFactor": 1.0,
        },
    }

def make_cube_mesh(name, face_groups, face_indices, mat_offset):
    # One primitive per face group; material encodes brightness, extras carries group name
    # accessor layout: 0=pos, 1=tex, 2=norm, then 3..3+n-1 = per-group indices
    primitives = []
    for k, group_name in enumerate(face_groups):
        brightness = FACE_BRIGHTNESS.get(group_name, 'bright')
        mat_idx = mat_offset + BRIGHTNESS_INDEX[brightness]
        primitives.append({
            "attributes": {"POSITION": 0, "TEXCOORD_0": 1, "NORMAL": 2},
            "indices": 3 + k,
            "material": mat_idx,
            "extras": {"name": group_name},
        })
    return {"name": name, "primitives": primitives}


base = os.path.dirname(os.path.abspath(__file__))

cube_pos_raw, cube_tex_raw, cube_norm_raw, groups = parse_obj_grouped(
    os.path.join(base, "unit_cube.obj")
)
face_groups = list(groups.keys())   # preserves OBJ group order

pos, tex, norm, face_indices = build_cube_mesh(
    cube_pos_raw, cube_tex_raw, cube_norm_raw, groups
)

cube_data, offsets, lengths = build_cube_buffer(pos, tex, norm, face_groups, face_indices)

with open(os.path.join(base, "unit_cube.bin"), 'wb') as f:
    f.write(cube_data)
print(f"Written: unit_cube.bin ({len(cube_data)} bytes, {len(pos)} verts, {len(face_groups)} face groups)")
for g in face_groups:
    print(f"  {g}: {len(face_indices[g])//3} tris  brightness={FACE_BRIGHTNESS.get(g)}")

# bufferViews: pos, tex, norm, then one per face group
n_face = len(face_groups)
n_sections = 3 + n_face
bv_targets = [34962, 34962, 34962] + [34963] * n_face

pmin, pmax = bounds(pos)

accessor_defs = [
    {"componentType": 5126, "count": len(pos), "type": "VEC3", "min": pmin, "max": pmax},
    {"componentType": 5126, "count": len(tex), "type": "VEC2"},
    {"componentType": 5126, "count": len(norm), "type": "VEC3"},
]
for g in face_groups:
    accessor_defs.append({"componentType": 5123, "count": len(face_indices[g]), "type": "SCALAR"})

# Materials:
#   0-2:  cube_default bright/mid/dim  (placeholder gray, updated at runtime)
#   3-5:  cube_red bright/mid/dim
#   6-8:  cube_green bright/mid/dim
materials = [
    make_material("cube_default_bright", 0.0,   0.0,   1.0),
    make_material("cube_default_mid",    0.0,   0.0,   0.5),
    make_material("cube_default_dim",    0.0,   0.0,   0.25),
    make_material("cube_red_bright",     1.0,   0.0,   0.0),
    make_material("cube_red_mid",        0.5,   0.0,   0.0),
    make_material("cube_red_dim",        0.25,  0.0,   0.0),
    make_material("cube_green_bright",   0.0,   1.0,   0.0),
    make_material("cube_green_mid",      0.0,   0.5,   0.0),
    make_material("cube_green_dim",      0.0,   0.25,  0.0),
]

# Meshes: cube_default (mat offset 0), cube_red (3), cube_green (6)
meshes = [
    make_cube_mesh("cube_default", face_groups, face_indices, 0),
    make_cube_mesh("cube_red",     face_groups, face_indices, 3),
    make_cube_mesh("cube_green",   face_groups, face_indices, 6),
]

# Nodes: 28 cubes at positions from Scene.cpp draw() with m_nSide=7
#   for i in 0..6: z = (7 - i - 0.5)
#     for j in 0..i: x = i - j + 0.5, y = j + 0.5
#   special: (i=6,j=0) -> cube_red mesh, (i=6,j=6) -> cube_green mesh
n_side = 7
nodes = []
for i in range(n_side):
    z = float(n_side - i) - 0.5
    for j in range(i + 1):
        x = float(i - j) + 0.5
        y = float(j) + 0.5
        if i == n_side - 1 and j == 0:
            mesh_idx = 1
            name = "cube_red"
        elif i == n_side - 1 and j == i:
            mesh_idx = 2
            name = "cube_green"
        else:
            mesh_idx = 0
            name = f"cube_{i}_{j}"
        nodes.append({"name": name, "mesh": mesh_idx, "translation": sw_t(x, y, z)})


def write_gltf(filepath, buffer_entry, bv_list, acc_list):
    gltf = {
        "asset": {"version": "2.0", "generator": "quetzal cubes"},
        "scene": 0,
        "scenes": [{"name": "cubes", "nodes": list(range(len(nodes)))}],
        "nodes": nodes,
        "meshes": meshes,
        "materials": materials,
        "accessors": acc_list,
        "bufferViews": bv_list,
        "buffers": [buffer_entry],
    }
    with open(filepath, 'w', newline='\n') as f:
        json.dump(gltf, f, indent=2)


# --- cubes.gltf: embedded base64 ---

b64_uri = 'data:application/octet-stream;base64,' + base64.b64encode(cube_data).decode()
b64_views = [{"buffer": 0, "byteOffset": offsets[k], "byteLength": lengths[k], "target": bv_targets[k]} for k in range(n_sections)]
b64_accs  = [{"bufferView": k, "byteOffset": 0, **accessor_defs[k]} for k in range(n_sections)]

write_gltf(
    os.path.join(base, "cubes.gltf"),
    {"byteLength": len(cube_data), "uri": b64_uri},
    b64_views, b64_accs
)
print(f"Written: cubes.gltf ({len(cube_data)} bytes embedded, {len(nodes)} nodes)")

# --- cubes_bin.gltf: references unit_cube.bin ---

bin_views = [{"buffer": 0, "byteOffset": offsets[k], "byteLength": lengths[k], "target": bv_targets[k]} for k in range(n_sections)]
bin_accs  = [{"bufferView": k, "byteOffset": 0, **accessor_defs[k]} for k in range(n_sections)]

write_gltf(
    os.path.join(base, "cubes_bin.gltf"),
    {"byteLength": len(cube_data), "uri": "unit_cube.bin"},
    bin_views, bin_accs
)
print(f"Written: cubes_bin.gltf")
