//------------------------------------------------------------------------------
// dungen
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/dungen/generate.hpp"
#include "quetzal/dungen/write_zone.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include "quetzal/log/Log.hpp"
#include "quetzal/log/StreamLogSink.hpp"
#include "quetzal/stratum_format/write_zone.hpp"
#include <exception>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

using namespace std;
using namespace quetzal;

namespace
{

    //--------------------------------------------------------------------------
    log::LogLevel to_level(const string& s)
    {
        optional<log::LogLevel> level = log::to_log_level(s);
        if (!level)
        {
            throw invalid_argument(format("\"{}\" is not a log level", s));
        }

        return *level;
    }

    //--------------------------------------------------------------------------
    void usage(ostream& os)
    {
        os << "Usage: dungen [zone] [algorithm=maze1|grid1|cave1|surface1] [extent=nx,ny] [name=text]"
            << " [stairs_down=min-max] [stairs_up=min-max] [stairs_down_region_spec=name] [room_density=d] [door_probability=p]"
            << " [position=x,y,z] [seed=n]"
            << " [log_level=trace|debug|info|warning|error|critical|off]" << endl;
        os << "  maze1: [room_dimension_base=n] [room_dimension_variation=n] [room_merge=true|false]"
            << " [corridor_straightness=s] [probability_dead_end_removal=p]"
            << " [probability_loop_removal=p]" << endl;
        os << "  grid1: [divisions=nx,ny] [room_dimension_min=n] [room_margin=n] [loop=l]" << endl;
        os << "  cave1: [fill=f] [smoothing_passes=n] [birth_threshold=n]"
            << " [survival_threshold=n]" << endl;
        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    string filepath = "dungen.zone";
    KeyValueStore tokens;
    log::logger().add_sink(log::StreamLogSink(clog));

    try
    {
        for (int i = 1; i < argc; ++i)
        {
            string arg = argv[i];
            size_t equals = arg.find('=');

            if (equals != string::npos)
            {
                tokens.insert(arg.substr(0, equals), arg.substr(equals + 1));
            }
            else if (arg.starts_with("-"))
            {
                usage(cerr);
                return 1;
            }
            else
            {
                filepath = arg;
            }
        }

        string name = tokens.value_or("name", string());
        tokens.remove("name");

        if (optional<string> level = tokens.value("log_level"))
        {
            log::logger().set_level(to_level(*level));
            tokens.remove("log_level");
        }

        dungen::Parameters parameters;
        dungen::set_parameters(parameters, tokens);

        // A random seed when none is given, written to the header so the zone can be reproduced
        if (!parameters.seed)
        {
            parameters.seed = dungen::random_seed();
        }

        dungen::Zone zone = dungen::generate(0, name, parameters);

        ofstream os(filepath, ios_base::binary);
        if (!os)
        {
            throw runtime_error(format("unable to open zone file {}", filepath));
        }

        stratum_format::Document document = dungen::make_document(zone, parameters);
        document.is_unpopulated = true;

        stratum_format::write(os, document);
        stratum_format::write(cout, document);
    }
    catch (const exception& e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
