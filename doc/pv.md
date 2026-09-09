
type	components	arithmetic	ordering	dims
stratum_sim::GridPosition	long i, j	affine only	== only	2
stratum_sim::detail::Vertex	int i, j	none	defaulted <=>	2
catagen::CellPosition	int i, j	none	defaulted <=>	2
catagen::Extent	int nx, ny	none	== only	2
dungen position/vector/dimensions	DiscreteVector<long, 2>, all three	full vector algebra	<=>	2
stratum_view::Buffer, Span	bare size_t i, j	none	none	2

Replace stratum_sim::GridPosition with DiscretePoint. Call the variables of that type 'position' instead of 'cell'.


Converted and clean: dungen and catagen both now use point_type = DiscretePoint<long, 2> and vector_type = DiscreteVector<long, 2>.

What is left:

type	where	shape	uses	duplicates
GridPosition	stratum_sim/types.hpp:55	long i, j	71, across 12 files incl. both hosts	DiscretePoint<long, 2>
detail::Vertex	load_layout.hpp:91	int i, j, defaulted <=>	36, across 4 loader files	DiscretePoint<int, 2>
Buffer / Span	stratum_view/Buffer.hpp	bare size_t i, j; Span { size_t i, j, count }	throughout the view	no type at all — unsigned, unpaired
Rectangle	dungen/types.hpp:24 and catagen/types.hpp:20	point_type position; vector_type extent;	both generators	two identical copies of one struct
Item::point_type	dungen/Item.hpp:30	Point<CartesianTraits<float, 3>>	Item only	shadows dungen's own point_type, same name, different type
discrete_point_type	stratum_sim/types.hpp:35	alias	none	dead — declared and never referenced
Two notes on the extent side. DiscreteLayout::extent is already discrete_vector_type, so the layout's cell counts are on the shared type. Buffer is the one place a size is still two loose size_t parameters rather than a vector.

And a name clash rather than a type clash: stratum_sim now has two Vertex — detail::Vertex (a coordinate) and Room::Vertex (position, wall id and type, 5 uses). Only the first is a duplicate; the second just makes the name ambiguous when reading.