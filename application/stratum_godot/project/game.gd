# The 2D tile view, drawn from place types, with the menu that creates, continues, saves, opens and quits games

extends Node2D

const TILE_SIZE := 16

# The most screen pixels a tile's side is drawn at; a zone too large for the screen is drawn smaller
const TILE_SCALE_MAX := 2.0

# The status line's height in the window, above the zone
const STATUS_HEIGHT := 32

# The window's title bar and border, which its size does not include, are kept within the usable screen
const WINDOW_FRAME_SIZE := Vector2i(16, 48)

# Flat color tiles, indexed by StratumWorld.FeatureType, with an entry for every value
const FEATURE_COLORS: Array[Color] = [
	Color(0.00, 0.00, 0.00, 0.0),  # none, not emitted
	Color(0.13, 0.13, 0.16),       # solid
	Color(0.55, 0.52, 0.45),       # floor
	Color(0.30, 0.28, 0.26),       # wall, not emitted
	Color(0.70, 0.45, 0.20),       # door
	Color(0.35, 0.60, 0.80),       # staircase
	Color(0.85, 0.72, 0.50),       # door open
	Color(0.20, 0.30, 0.60),       # staircase down
]

# The actor tiles follow the feature tiles, indexed by StratumWorld.Occupant less one, and are painted on a layer above them
const ACTOR_COLORS: Array[Color] = [
	Color(0.95, 0.80, 0.25),       # the player
	Color(0.85, 0.15, 0.15),       # any other actor
]

# Passed after -- on Godot's command line, it starts a new game without showing the menu, for the headless check
const NEW_GAME_ARGUMENT := "--new-game"

# A digit opens a save, so the saves page lists no more than nine
const SAVED_GAMES_LISTED_MAX := 9

var _world: StratumWorld
var _layer: TileMapLayer
var _actor_layer: TileMapLayer
var _status: Label
var _camera: Camera2D
var _menu: Control
var _menu_text: Label
var _extent := Vector2i.ZERO

# The saves the saves page lists, newest first, while it is shown, and empty while the main page is
var _saves_listed: Array = []
var _saves_page_shown := false
var _save_selected := 0

# The next key answers whether to delete the selected save
var _save_removal_confirming := false


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
	_camera.zoom = Vector2(TILE_SCALE_MAX, TILE_SCALE_MAX)
	add_child(_camera)
	_camera.make_current()

	if NEW_GAME_ARGUMENT in OS.get_cmdline_user_args():
		_new_game()
		print("tile view: ", _extent, " cells, ", _layer.get_used_cells().size(), " painted from ",
			_layer.tile_set.get_source(0).get_tiles_count(), " tiles, in a window of ", get_window().size)
	else:
		_show_menu("")
		print("tile view: menu shown")


func _build_tile_set() -> TileSet:
	var colors := FEATURE_COLORS + ACTOR_COLORS
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
		lines.append("S   Save")

	lines.append("O   Open")
	lines.append("T   Notation  " + _world.notation())
	lines.append("V   View      " + _world.view())
	lines.append("Q   Quit")

	if not message.is_empty():
		lines.append_array(["", message])

	_saves_page_shown = false
	_saves_listed = []
	_menu_text.text = "\n".join(lines)
	_menu.visible = true


# Each save is shown by its character's name and the time of the save, never by its directory's name
func _show_saves(message: String) -> void:
	_saves_listed = _world.saved_games().slice(0, SAVED_GAMES_LISTED_MAX)
	_save_selected = clampi(_save_selected, 0, maxi(_saves_listed.size() - 1, 0))
	_draw_saves(message)


func _draw_saves(message: String) -> void:
	var lines := ["OPEN", ""]

	for i in _saves_listed.size():
		lines.append("%s %d   %s" % [">" if i == _save_selected else "  ", i + 1, _save_label(_saves_listed[i])])

	if _saves_listed.is_empty():
		lines.append("No saved games")

	lines.append_array(["", message if not message.is_empty()
		else "Up and Down select, Enter or a digit opens, Delete deletes, Escape goes back"])

	_saves_page_shown = true
	_menu_text.text = "\n".join(lines)
	_menu.visible = true


func _save_label(save: Dictionary) -> String:
	return "%s   %s   %s" % [save["character_name"], save["time"], save["save_type"]]


func _new_game() -> void:
	if not _world.create_game():
		_show_menu("No game was created; Godot's error list has the reason")
		return

	_start_play()


func _open_game(save: Dictionary) -> void:
	if not _world.open_game(save["save_name"]):
		_show_menu("The game was not opened; Godot's error list has the reason")
		return

	_start_play()


func _start_play() -> void:
	_saves_page_shown = false
	_saves_listed = []
	_menu.visible = false
	_refresh()


func _refresh() -> void:
	# A game that fails during play is quit
	if not _world.has_game():
		_show_menu("The game ended in an error; Godot's error list has the reason")
		return

	# The player may have entered a zone of another size
	var extent := _world.extent()
	if extent != _extent:
		_extent = extent
		_layer.clear()
		_actor_layer.clear()
		_fit_window()

	var cells := _world.cells()
	var actors := _world.actors()

	for j in _extent.y:
		for i in _extent.x:
			var slot := j * _extent.x + i

			# Row zero is the lowest in the simulation and y grows downward in Godot
			var at := Vector2i(i, _extent.y - 1 - j)

			_layer.set_cell(at, 0, Vector2i(cells[slot], 0))

			if actors[slot] != StratumWorld.OccupantNone:
				_actor_layer.set_cell(at, 0, Vector2i(FEATURE_COLORS.size() + actors[slot] - 1, 0))
			else:
				_actor_layer.erase_cell(at)

	_status.text = "cell %s   turn %d   %s" % [_world.cell(), _world.turn(), _world.message()]

	if _world.game_over():
		_status.text += "   Press any key."


# Sizes the window to the zone below the status line, and centers it on the screen
func _fit_window() -> void:
	var zone_size := Vector2(_extent.x, _extent.y) * TILE_SIZE
	var tile_scale := TILE_SCALE_MAX

	# A headless run has no screen
	var usable := DisplayServer.screen_get_usable_rect().size - WINDOW_FRAME_SIZE
	if usable.x > 0 and usable.y > STATUS_HEIGHT:
		tile_scale = minf(tile_scale, minf(usable.x / zone_size.x, (usable.y - STATUS_HEIGHT) / zone_size.y))

	_camera.zoom = Vector2(tile_scale, tile_scale)
	_camera.position = zone_size * 0.5 - Vector2(0, STATUS_HEIGHT * 0.5 / tile_scale)
	get_window().size = Vector2i(zone_size * tile_scale) + Vector2i(0, STATUS_HEIGHT)
	get_window().move_to_center()


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

	if _saves_page_shown:
		_saves_key(event)
		return

	match event.keycode:
		KEY_N:
			_new_game()
		KEY_C:
			if _world.has_game():
				_world.menu_close()
				_menu.visible = false
		KEY_S:
			if _world.has_game():
				var saved: bool = not _world.save_game().is_empty()
				_show_menu("Game saved" if saved else "The game was not saved; Godot's error list has the reason")
		KEY_O:
			_save_selected = 0
			_show_saves("")
		KEY_T:
			_world.cycle_notation()
			_show_menu("")
		KEY_V:
			_world.cycle_view()
			_show_menu("")
		KEY_Q, KEY_ESCAPE:
			_world.quit_game()
			get_tree().quit()


func _saves_key(event: InputEventKey) -> void:
	# Any key but Y answers no
	if _save_removal_confirming:
		_save_removal_confirming = false

		if event.keycode == KEY_Y:
			var removed: bool = _world.remove_game(_saves_listed[_save_selected]["save_name"])
			_show_saves("Save deleted" if removed else "The save was not deleted; Godot's error list has the reason")
		else:
			_draw_saves("")

		return

	if event.keycode == KEY_ESCAPE:
		_show_menu("")
		return

	if _saves_listed.is_empty():
		return

	match event.keycode:
		KEY_UP, KEY_K:
			_save_selected = maxi(_save_selected - 1, 0)
			_draw_saves("")
		KEY_DOWN, KEY_J:
			_save_selected = mini(_save_selected + 1, _saves_listed.size() - 1)
			_draw_saves("")
		KEY_ENTER, KEY_KP_ENTER:
			_open_game(_saves_listed[_save_selected])
		KEY_DELETE:
			_save_removal_confirming = true
			_draw_saves("Delete %s? Y or N" % _save_label(_saves_listed[_save_selected]))
		_:
			var slot := event.keycode - KEY_1
			if slot >= 0 and slot < _saves_listed.size():
				_open_game(_saves_listed[slot])


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

	# < or > enters the access point on the floor of the player's cell. A key with no character has the code point 0
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
