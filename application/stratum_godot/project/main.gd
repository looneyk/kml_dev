# Headless check that the extension loads and the simulation runs inside Godot:
#   godot --headless --path project --script main.gd

extends SceneTree

func _initialize() -> void:
	var world := StratumWorld.new()

	if not world.create_game():
		push_error("create_game failed")
		quit(1)
		return

	print("create_game -> ok")

	print("extent -> ", world.extent())
	print("start  -> ", world.cell())

	# East twice, then north. A blocked move must not advance the turn
	print("step E -> ", world.step(1, 0), "  turn ", world.turn())
	print("step E -> ", world.step(1, 0), "  turn ", world.turn())
	print("step N -> ", world.step(0, 1), "  turn ", world.turn())

	var frame: String = world.render()
	var lines := frame.split("\n", false)
	print("render -> ", lines.size(), " rows, ", lines[0].length(), " columns")

	var codes := world.cells()
	var actors := world.actors()
	print("cells  -> ", codes.size(), " codes, ", codes.count(StratumWorld.PlaceTypeFloor), " floor, ",
		actors.count(StratumWorld.OccupantPlayer), " player, ", actors.count(StratumWorld.OccupantOther), " other")

	quit()
