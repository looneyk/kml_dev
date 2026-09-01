# Headless check that the extension loads and the simulation runs inside Godot:
#   godot --headless --path project --script main.gd

extends SceneTree

func _initialize() -> void:
	var world := StratumWorld.new()

	var path := world.default_level_path()
	print("default_level_path -> ", path)

	if not world.load_level(path):
		push_error("load_level failed")
		quit(1)
		return

	print("extent -> ", world.extent())
	print("start  -> ", world.cell())

	# East twice, then north, the same sequence the terminal host's reference frames use.
	# The blocked step must not advance the turn, the same as in the terminal host
	print("step E -> ", world.step(1, 0), "  turn ", world.turn())
	print("step E -> ", world.step(1, 0), "  turn ", world.turn())
	print("step N -> ", world.step(0, 1), "  turn ", world.turn())

	var frame: String = world.render()
	var lines := frame.split("\n", false)
	print("render -> ", lines.size(), " rows, ", lines[0].length(), " columns")

	var codes := world.cells()
	print("cells  -> ", codes.size(), " codes, ", codes.count(StratumWorld.KindFloor), " floor, ",
		codes.count(StratumWorld.KindActor), " actor")

	quit()
