# The 2D tile view. Consumes cell kinds rather than the characters the terminal host draws,
# which is the whole reason for having a second view at all.

extends Node2D

const TILE_SIZE := 16

# Built here rather than imported, so the repository carries no binary art for what is three
# flat colours. A real tileset replaces this without anything else changing
const KIND_COLORS: Array[Color] = [
	Color(0.13, 0.13, 0.16),  # rock
	Color(0.55, 0.52, 0.45),  # floor
	Color(0.95, 0.80, 0.25),  # actor
]

var _world: StratumWorld
var _layer: TileMapLayer
var _status: Label
var _extent: Vector2i


func _ready() -> void:
	_world = StratumWorld.new()

	if not _world.load_zone(_world.default_zone_path()):
		push_error("could not load the zone")
		return

	_extent = _world.extent()

	_layer = TileMapLayer.new()
	_layer.name = "Tiles"
	_layer.tile_set = _build_tile_set()
	add_child(_layer)

	# On its own layer so it stays put while the camera moves
	var ui := CanvasLayer.new()
	add_child(ui)
	_status = Label.new()
	_status.position = Vector2(8, 8)
	ui.add_child(_status)

	var camera := Camera2D.new()
	camera.position = Vector2(_extent.x, _extent.y) * TILE_SIZE * 0.5
	camera.zoom = Vector2(2.0, 2.0)
	add_child(camera)
	camera.make_current()

	_refresh()

	print("tile view: ", _extent, " cells, ", _layer.get_used_cells().size(), " painted from ",
		_layer.tile_set.get_source(0).get_tiles_count(), " tiles")


func _build_tile_set() -> TileSet:
	var image := Image.create(TILE_SIZE * KIND_COLORS.size(), TILE_SIZE, false, Image.FORMAT_RGBA8)

	for kind in KIND_COLORS.size():
		image.fill_rect(Rect2i(kind * TILE_SIZE, 0, TILE_SIZE, TILE_SIZE), KIND_COLORS[kind])

	var source := TileSetAtlasSource.new()
	source.texture = ImageTexture.create_from_image(image)
	source.texture_region_size = Vector2i(TILE_SIZE, TILE_SIZE)

	for kind in KIND_COLORS.size():
		source.create_tile(Vector2i(kind, 0))

	var tile_set := TileSet.new()
	tile_set.tile_size = Vector2i(TILE_SIZE, TILE_SIZE)
	tile_set.add_source(source, 0)

	return tile_set


func _refresh() -> void:
	var cells := _world.cells()

	for j in _extent.y:
		for i in _extent.x:
			var kind := cells[j * _extent.x + i]

			# Row zero is the lowest in the simulation and y grows downward in Godot
			_layer.set_cell(Vector2i(i, _extent.y - 1 - j), 0, Vector2i(kind, 0))

	_status.text = "cell %s   turn %d" % [_world.cell(), _world.turn()]


func _unhandled_key_input(event: InputEvent) -> void:
	if not event.is_pressed() or event.is_echo():
		return

	var step := Vector2i.ZERO

	match (event as InputEventKey).keycode:
		KEY_RIGHT, KEY_L: step = Vector2i(1, 0)
		KEY_LEFT, KEY_H: step = Vector2i(-1, 0)
		KEY_UP, KEY_K: step = Vector2i(0, 1)
		KEY_DOWN, KEY_J: step = Vector2i(0, -1)
		KEY_ESCAPE, KEY_Q: get_tree().quit()
		_: return

	if step != Vector2i.ZERO:
		_world.step(step.x, step.y)
		_refresh()

	get_viewport().set_input_as_handled()
