import json
import struct
import base64
import math
import os

def parse_obj(filepath):
    positions = []
    texcoords = []
    normals = []
    flat_verts = []  # list of (pos_idx, tex_idx, norm_idx) per triangle vertex

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
            elif line.startswith('f '):
                parts = line.split()[1:]
                face = []
                for part in parts:
                    idx = part.split('/')
                    v = int(idx[0]) - 1
                    t = int(idx[1]) - 1 if len(idx) > 1 and idx[1] else -1
                    n = int(idx[2]) - 1 if len(idx) > 2 and idx[2] else -1
                    face.append((v, t, n))
                for i in range(1, len(face) - 1):
                    flat_verts.extend([face[0], face[i], face[i + 1]])

    return positions, texcoords, normals, flat_verts

def build_mesh(positions, texcoords, normals, flat_verts):
    vertex_map = {}
    out_pos = []
    out_tex = []
    out_norm = []
    out_idx = []

    for (vi, ti, ni) in flat_verts:
        key = (vi, ti, ni)
        if key not in vertex_map:
            vertex_map[key] = len(out_pos)
            out_pos.append(sw_t(*positions[vi]))
            out_tex.append(texcoords[ti] if 0 <= ti < len(texcoords) else (0.0, 0.0))
            out_norm.append(sw_t(*normals[ni]) if 0 <= ni < len(normals) else (0.0, 0.0, 1.0))
        out_idx.append(vertex_map[key])

    return out_pos, out_tex, out_norm, out_idx

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
def sw_s(sx, sy, sz): return [sx, sz, sy]
def sw_q(qx, qy, qz, qw): return [qx, qz, -qy, qw]

def bounds(positions):
    xs = [p[0] for p in positions]
    ys = [p[1] for p in positions]
    zs = [p[2] for p in positions]
    return [min(xs), min(ys), min(zs)], [max(xs), max(ys), max(zs)]

def build_buffer(p, t, n, i):
    """Pack positions, texcoords, normals, indices; return (data, offsets, lengths)."""
    parts = [pack_vec3(p), pack_vec2(t), pack_vec3(n), pack_u16(i)]
    offsets = []
    lengths = []
    data = b''
    for part in parts:
        offsets.append(len(data))
        lengths.append(len(part))
        data += pad4(part)
    return data, offsets, lengths

def make_axes_gltf(meshes_data, buffers_spec, materials, gltf_meshes, nodes):
    """Build the glTF dict given per-mesh buffer specs."""
    buffer_views = []
    accessors = []

    for m_idx, ((p, t, n, i), (buf_idx, offsets, lengths)) in enumerate(zip(meshes_data, buffers_spec)):
        bv_base = m_idx * 4
        buffer_views.append({"buffer": buf_idx, "byteOffset": offsets[0], "byteLength": lengths[0], "target": 34962})
        buffer_views.append({"buffer": buf_idx, "byteOffset": offsets[1], "byteLength": lengths[1], "target": 34962})
        buffer_views.append({"buffer": buf_idx, "byteOffset": offsets[2], "byteLength": lengths[2], "target": 34962})
        buffer_views.append({"buffer": buf_idx, "byteOffset": offsets[3], "byteLength": lengths[3], "target": 34963})
        pmin, pmax = bounds(p)
        accessors.append({"bufferView": bv_base,   "byteOffset": 0, "componentType": 5126, "count": len(p), "type": "VEC3", "min": pmin, "max": pmax})
        accessors.append({"bufferView": bv_base+1, "byteOffset": 0, "componentType": 5126, "count": len(t), "type": "VEC2"})
        accessors.append({"bufferView": bv_base+2, "byteOffset": 0, "componentType": 5126, "count": len(n), "type": "VEC3"})
        accessors.append({"bufferView": bv_base+3, "byteOffset": 0, "componentType": 5123, "count": len(i), "type": "SCALAR"})

    return {
        "asset": {"version": "2.0", "generator": "quetzal axes"},
        "scene": 0,
        "scenes": [{"name": "axes", "nodes": list(range(len(nodes)))}],
        "nodes": nodes,
        "meshes": gltf_meshes,
        "materials": materials,
        "accessors": accessors,
        "bufferViews": buffer_views,
    }


base = os.path.dirname(os.path.abspath(__file__))

cyl_pos, cyl_tex, cyl_norm, cyl_flat = parse_obj(os.path.join(base, "unit_cylinder.obj"))
cone_pos, cone_tex, cone_norm, cone_flat = parse_obj(os.path.join(base, "unit_cone.obj"))
sph_pos, sph_tex, sph_norm, sph_flat = parse_obj(os.path.join(base, "unit_geosphere2.obj"))

cyl_p, cyl_t, cyl_n, cyl_i = build_mesh(cyl_pos, cyl_tex, cyl_norm, cyl_flat)
cone_p, cone_t, cone_n, cone_i = build_mesh(cone_pos, cone_tex, cone_norm, cone_flat)
sph_p, sph_t, sph_n, sph_i = build_mesh(sph_pos, sph_tex, sph_norm, sph_flat)

meshes_data = [
    (cyl_p, cyl_t, cyl_n, cyl_i),
    (cone_p, cone_t, cone_n, cone_i),
    (sph_p, sph_t, sph_n, sph_i),
]

# Materials: white (origin), red (X), green (Y), blue (Z)
materials = [
    {"name": "white", "emissiveFactor": [1.0, 1.0, 1.0], "pbrMetallicRoughness": {"baseColorFactor": [0.0, 0.0, 0.0, 1.0], "metallicFactor": 0.0, "roughnessFactor": 1.0}},
    {"name": "red",   "emissiveFactor": [1.0, 0.0, 0.0], "pbrMetallicRoughness": {"baseColorFactor": [0.0, 0.0, 0.0, 1.0], "metallicFactor": 0.0, "roughnessFactor": 1.0}},
    {"name": "green", "emissiveFactor": [0.0, 1.0, 0.0], "pbrMetallicRoughness": {"baseColorFactor": [0.0, 0.0, 0.0, 1.0], "metallicFactor": 0.0, "roughnessFactor": 1.0}},
    {"name": "blue",  "emissiveFactor": [0.0, 0.0, 1.0], "pbrMetallicRoughness": {"baseColorFactor": [0.0, 0.0, 0.0, 1.0], "metallicFactor": 0.0, "roughnessFactor": 1.0}},
]

# Meshes: reuse cyl/cone geometry accessors (4 per mesh) with per-axis materials
# accessor layout per mesh: pos=0, tex=1, norm=2, idx=3
# cyl:  0,1,2,3   cone: 4,5,6,7   sph: 8,9,10,11
gltf_meshes = [
    {"name": "sphere_white",  "primitives": [{"attributes": {"POSITION": 8, "TEXCOORD_0": 9, "NORMAL": 10}, "indices": 11, "material": 0}]},
    {"name": "cylinder_red",  "primitives": [{"attributes": {"POSITION": 0, "TEXCOORD_0": 1, "NORMAL": 2},  "indices": 3,  "material": 1}]},
    {"name": "cone_red",      "primitives": [{"attributes": {"POSITION": 4, "TEXCOORD_0": 5, "NORMAL": 6},  "indices": 7,  "material": 1}]},
    {"name": "cylinder_green","primitives": [{"attributes": {"POSITION": 0, "TEXCOORD_0": 1, "NORMAL": 2},  "indices": 3,  "material": 2}]},
    {"name": "cone_green",    "primitives": [{"attributes": {"POSITION": 4, "TEXCOORD_0": 5, "NORMAL": 6},  "indices": 7,  "material": 2}]},
    {"name": "cylinder_blue", "primitives": [{"attributes": {"POSITION": 0, "TEXCOORD_0": 1, "NORMAL": 2},  "indices": 3,  "material": 3}]},
    {"name": "cone_blue",     "primitives": [{"attributes": {"POSITION": 4, "TEXCOORD_0": 5, "NORMAL": 6},  "indices": 7,  "material": 3}]},
]

# Node transforms matching Axes.cpp draw() with scale=1.0, position=(0,0,0):
#   origin sphere:  scale 0.05 uniform
#   X shaft: scale [0.02,0.02,1.0], rotateY(+pi/2)
#   X head:  scale [0.05,0.05,0.2], rotateY(+pi/2), translate [1,0,0]
#   Y shaft: scale [0.02,0.02,1.0], rotateX(-pi/2)
#   Y head:  scale [0.05,0.05,0.2], rotateX(-pi/2), translate [0,1,0]
#   Z shaft: scale [0.02,0.02,1.0]
#   Z head:  scale [0.05,0.05,0.2], translate [0,0,1]
# Quaternions in glTF (x,y,z,w):
#   rotateY(+pi/2): [0, sin(pi/4), 0, cos(pi/4)]
#   rotateX(-pi/2): [sin(-pi/4), 0, 0, cos(pi/4)]
s = math.sqrt(2.0) / 2.0

nodes = [
    {"name": "origin",  "mesh": 0, "scale": sw_s(0.05, 0.05, 0.05)},
    {"name": "x_shaft", "mesh": 1, "scale": sw_s(0.02, 0.02, 1.0), "rotation": sw_q(0.0, s, 0.0, s)},
    {"name": "x_head",  "mesh": 2, "scale": sw_s(0.05, 0.05, 0.2), "rotation": sw_q(0.0, s, 0.0, s), "translation": sw_t(1.0, 0.0, 0.0)},
    {"name": "y_shaft", "mesh": 3, "scale": sw_s(0.02, 0.02, 1.0), "rotation": sw_q(-s, 0.0, 0.0, s)},
    {"name": "y_head",  "mesh": 4, "scale": sw_s(0.05, 0.05, 0.2), "rotation": sw_q(-s, 0.0, 0.0, s), "translation": sw_t(0.0, 1.0, 0.0)},
    {"name": "z_shaft", "mesh": 5, "scale": sw_s(0.02, 0.02, 1.0)},
    {"name": "z_head",  "mesh": 6, "scale": sw_s(0.05, 0.05, 0.2), "translation": sw_t(0.0, 0.0, 1.0)},
]

# --- axes.gltf: single embedded base64 buffer ---

raw_parts = []
for p, t, n, i in meshes_data:
    raw_parts.extend([pack_vec3(p), pack_vec2(t), pack_vec3(n), pack_u16(i)])

b64_offsets = []
b64_lengths = []
b64_data = b''
for part in raw_parts:
    b64_offsets.append(len(b64_data))
    b64_lengths.append(len(part))
    b64_data += pad4(part)

b64_uri = 'data:application/octet-stream;base64,' + base64.b64encode(b64_data).decode()
b64_specs = [(0, b64_offsets[m*4:m*4+4], b64_lengths[m*4:m*4+4]) for m in range(3)]

gltf_b64 = make_axes_gltf(meshes_data, b64_specs, materials, gltf_meshes, nodes)
gltf_b64["buffers"] = [{"byteLength": len(b64_data), "uri": b64_uri}]

with open(os.path.join(base, "axes.gltf"), 'w', newline='\n') as f:
    json.dump(gltf_b64, f, indent=2)

print(f"Written: axes.gltf ({len(b64_data)} bytes embedded)")

# --- axes_bin.gltf: separate .bin file per OBJ ---

bin_names = ["unit_cylinder.bin", "unit_cone.bin", "unit_geosphere2.bin"]
bin_buffers = []
bin_specs = []

for m_idx, ((p, t, n, i), name) in enumerate(zip(meshes_data, bin_names)):
    data, offsets, lengths = build_buffer(p, t, n, i)
    with open(os.path.join(base, name), 'wb') as f:
        f.write(data)
    bin_buffers.append({"byteLength": len(data), "uri": name})
    bin_specs.append((m_idx, offsets, lengths))
    print(f"Written: {name} ({len(data)} bytes)")

gltf_bin = make_axes_gltf(meshes_data, bin_specs, materials, gltf_meshes, nodes)
gltf_bin["buffers"] = bin_buffers

with open(os.path.join(base, "axes_bin.gltf"), 'w', newline='\n') as f:
    json.dump(gltf_bin, f, indent=2)

print(f"Written: axes_bin.gltf")
print(f"Cylinder: {len(cyl_p)} verts, {len(cyl_i)//3} tris")
print(f"Cone:     {len(cone_p)} verts, {len(cone_i)//3} tris")
print(f"Sphere:   {len(sph_p)} verts, {len(sph_i)//3} tris")
