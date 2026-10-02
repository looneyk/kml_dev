# The 2D tile view, drawn from place types, with the menu that creates, continues and quits games

extends Node2D

const TILE_SIZE := 16

# Flat color tiles, indexed by StratumWorld.PlaceType, with an entry for every value
const PLACE_COLORS: Array[Color] = [
	Color(0.00, 0.00, 0.00, 0.0),  # none, not emitted
	Color(0.13, 0.13, 0.16),       # solid
	Color(0.55, 0.52, 0.45),       # floor
	Color(0.30, 0.28, 0.26),       # wall, not emitted
	Color(0.70, 0.45, 0.20),       # door
	Color(0.35, 0.60, 0.80),       # stairs up
	Color(0.20, 0.30, 0.60),       # stairs down
	Color(0.85, 0.72, 0.50),       # open door
]

# The actor tiles follow the place tiles, indexed by StratumWorld.Occupant less one, and are painted on a layer above them
const ACTOR_COLORS: Array[Color] = [
	Color(0.95, 0.80, 0.25),       # the player
	Color(0.85, 0.15, 0.15),       # any other actor
]

# Passed after -- on Godot's command line, it starts a new game without showing the menu, for the headless check
const NEW_GAME_ARGUMENT := "--new-game"

var _world: StratumWorld
var _layer: TileMapLayer
var _actor_layer: TileMapLayer
var _status: Label
var _camera: Camera2D
var _menu: Control
var _menu_text: Label
var _extent := Vector2i.ZERO


func _ready() -> void:
	_world = StratumWorld.new()

	var tile_set := _build_tile_set()

	_layer = TileMapLayer.new()
	_layer.name = "Tiles"
	_layer.tile_set = tile_set
	add_child(_layer)

	# Above the places
	_actor_layer = TileMapLayer.new()
	_actor_layer.name = "Actors"
	_actor_layer.tile_set = tile_set
	add_child(_actor_layer)

	# On its own layer so it stays put while the camera moves
	var ui := CanvasLayer.new()
	add_child(ui)
	_status = Label.new()
	_status.position = Vector2(8, 8)
	ui.add_child(_status)
	_build_menu(ui)

	_camera = Camera2D.new()
	_camera.zoom = Vector2(2.0, 2.0)
	add_child(_camera)
	_camera.make_current()

	if NEW_GAME_ARGUMENT in OS.get_cmdline_user_args():
		_new_game()
		print("tile view: ", _extent, " cells, ", _layer.get_used_cells().size(), " painted from ",
			_layer.tile_set.get_source(0).get_tiles_count(), " tiles")
	else:
		_show_menu("")
		print("tile view: menu shown")


func _build_tile_set() -> TileSet:
	var colors := PLACE_COLORS + ACTOR_COLORS
	var tiles := colors.size()
	var image := Image.create(TILE_SIZE * tiles, TILE_SIZE, false, Image.FORMAT_RGBA8)

	for tile in tiles:
		image.fill_rect(Rect2i(tile * TILE_SIZE, 0, TILE_SIZE, TILE_SIZE), colors[tile])

	var source := TileSetAtlasSource.new()
	source.texture = ImageTexture.create_from_image(image)
	source.texture_region_size = Vector2i(TILE_SIZE, TILE_SIZE)

	for tile in tiles:
		source.create_tile(Vector2i(tile, 0))

	var tile_set := TileSet.new()
	tile_set.tile_size = Vector2i(TILE_SIZE, TILE_SIZE)
	tile_set.add_source(source, 0)

	return tile_set


# A dark screen over the view, with the menu's choices on it
func _build_menu(ui: CanvasLayer) -> void:
	_menu = ColorRect.new()
	(_menu as ColorRect).color = Color(0.0, 0.0, 0.0, 0.85)
	_menu.set_anchors_preset(Control.PRESET_FULL_RECT)
	ui.add_child(_menu)

	_menu_text = Label.new()
	_menu_text.set_anchors_preset(Control.PRESET_CENTER)
	_menu_text.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_menu_text.grow_horizontal = Control.GROW_DIRECTION_BOTH
	_menu_text.grow_vertical = Control.GROW_DIRECTION_BOTH
	_menu.add_child(_menu_text)


func _show_menu(message: String) -> void:
	var lines := ["STRATUM", "", "N   New game"]

	if _world.has_game():
		lines.append("C   Continue")

	lines.append("Q   Quit")

	if not message.is_empty():
		lines.append_array(["", message])

	_menu_text.text = "\n".join(lines)
	_menu.visible = true


func _new_game() -> void:
	if not _world.create_game():
		_show_menu("No game was created; Godot's error list has the reason")
		return

	_menu.visible = false
	_refresh()


func _refresh() -> void:
	# The player may have entered a zone of another size
	var extent := _world.extent()
	if extent != _extent:
		_extent = extent
		_layer.clear()
		_actor_layer.clear()
		_camera.position = Vector2(_extent.x, _extent.y) * TILE_SIZE * 0.5

	var cells := _world.cells()
	var actors := _world.actors()

	for j in _extent.y:
		for i in _extent.x:
			var slot := j * _extent.x + i

			# Row zero is the lowest in the simulation and y grows downward in Godot
			var at := Vector2i(i, _extent.y - 1 - j)

			_layer.set_cell(at, 0, Vector2i(cells[slot], 0))

			if actors[slot] != StratumWorld.OccupantNone:
				_actor_layer.set_cell(at, 0, Vector2i(PLACE_COLORS.size() + actors[slot] - 1, 0))
			else:
				_actor_layer.erase_cell(at)

	_status.text = "cell %s   turn %d   %s" % [_world.cell(), _world.turn(), _world.message()]

	if _world.game_over():
		_status.text += "   Press any key."


func _unhandled_key_input(event: InputEvent) -> void:
	if not event.is_pressed():
		return

	# The death stays on screen until a key is pressed, and the game is then quit
	if not _menu.visible and _world.game_over():
		if not event.is_echo():
			_world.quit_game()
			_show_menu("")
	elif _menu.visible:
		_menu_key(event as InputEventKey)
	else:
		_play_key(event as InputEventKey)

	get_viewport().set_input_as_handled()


func _menu_key(event: InputEventKey) -> void:
	if event.is_echo():
		return

	match event.keycode:
		KEY_N:
			_new_game()
		KEY_C, KEY_ESCAPE:
			if _world.has_game():
				_world.menu_close()
				_menu.visible = false
		KEY_Q:
			_world.quit_game()
			get_tree().quit()


func _play_key(event: InputEventKey) -> void:
	# A held movement key repeats; any other key acts once per press
	var step := Vector2i.ZERO

	match event.keycode:
		KEY_RIGHT, KEY_L: step = Vector2i(1, 0)
		KEY_LEFT, KEY_H: step = Vector2i(-1, 0)
		KEY_UP, KEY_K: step = Vector2i(0, 1)
		KEY_DOWN, KEY_J: step = Vector2i(0, -1)

	if step != Vector2i.ZERO:
		_world.step(step.x, step.y)
		_refresh()
		return

	if event.is_echo():
		return

	# < or > enters the relocator on the floor of the player's cell. A key with no character has the code point 0
	if event.unicode in ["<".unicode_at(0), ">".unicode_at(0)]:
		_world.enter()
		_refresh()
		return

	match event.keycode:
		KEY_SPACE:
			_world.interact()
			_refresh()
		KEY_ESCAPE:
			_world.menu_open()
			_show_menu("")
