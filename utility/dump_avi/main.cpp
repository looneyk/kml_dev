//------------------------------------------------------------------------------
// dump_avi
// main.cpp
//------------------------------------------------------------------------------

#include "AVI.hpp"
#include "endian.hpp"
#include <windows.h>
#include <mmreg.h>
#include <iostream>
#include <cassert>
#include <exception>
#include <iomanip>
#include <cfloat>

using namespace std;
using namespace on2common;
using namespace AVI;

namespace
{
    AVI::File file;

    bool printIndexTogether = false;
    bool printIndexSeparate = false;
    int printExtendedIndex = -1;
    bool print_movi_chunks = false;
    bool printAnnotated = false;
    int fPrintKeyframes = -2;

    int depth = -1;

    bool s_check_indx = false;
    bool s_check_idx1 = false;

    void usage()
    {
        cout << VERSION_STRING << endl;
        cout << "usage: dump_avi [<options>] <infile>\n";
        cout << " -i1             print idx1 chunk\n";
        cout << " -is             print idx1 chunk, broken out by stream\n";
        cout << " -ix[<depth>]    print extended AVI index chunks\n";
        cout << " -ia             print idx1 annotated with time info\n";
        cout << " -k              list keyframes for all streams\n";
        cout << " -k<stream>      list keyframes for a single stream\n";
        cout << " -d[<depth>]     show chunks a la RIFFWALK\n";
        cout << " -m              print chunks inside LIST-movi chunks\n";
        cout << " -f1             compare idx1 size to frame size\n";
        cout << " -fx             compare indx size to frame size\n";
    }

    void printSuperIndexEntry(int i, const AVI::SuperIndexChunk::Entry& e)
    {
        cout << setw(2) << setfill(' ') << i << " ";

        const unsigned int offset_high = static_cast<unsigned int>(e.offset >> 32);
        const unsigned int offset_low = static_cast<unsigned int>(e.offset & 0xFFFFFFFFi64);

        cout << hex << setw(8) << setfill('0') << offset_high << " " << setw(8) << setfill('0') << offset_low << dec << "  ";
        cout << setw(10) << setfill(' ') << e.size << " ";
        cout << e.duration;
        cout << endl;

        if (printExtendedIndex > 0)
        {
            cout << endl;

            file.seek(e.offset);

            StandardIndexChunk chunk;

            file.read(&chunk, sizeof(IndexChunkEx)); //yes

            cout << chunk << endl;
            cout << setfill(' ');

            if (printExtendedIndex > 1)
            {
                for (AVI::dword i = 0; i < chunk.entriesInUse; ++i)
                {
                    StandardIndexChunk::Entry e;

                    file.read(&e, sizeof e);

                    cout << setw(8) << i << "  " << setw(12) << e.offset << "  " << setw(12) << e.size << endl;
                }

                cout << endl;
            }
        }
    }

    void printExtendedIndexInfo(int stream)
    {
        const SuperIndexChunk& sic = file.superIndexChunk(stream);

        cout << "Super Index for stream " << stream << endl;
        cout << endl;
        cout << sic;
        cout << endl;

        for (unsigned int i = 0; i < sic.entriesInUse; ++i)
            printSuperIndexEntry(i, sic.index[i]);
    }

    void printIndexEntry(const AVI::IndexEntry& ie)
    {
        cout << ie.chunkId << " ";
        cout << hex << setw(8) << setfill('0') << ie.chunkOffset << dec << " ";
        cout << setw(10) << setfill(' ') << ie.chunkLength << " ";
        cout << hex << setw(8) << setfill('0') << ie.flags << dec << " ";

        if (ie.flags & AVI::IndexEntry::keyframe)
        {
            cout << "key ";
        }
    }

    void printIndexChunk(int stream)
    {
        cout << endl;
        cout << "Index entries for stream " << stream << endl;
        cout << endl;

        AVI::IEVector iev;

        file.load(stream, iev);

        const int count = iev.size();

        for (int i = 0; i < count; ++i)
        {
            cout << setw(5) << setfill(' ') << i << " ";

            const AVI::IndexEntry& ie = iev.at(i);

            printIndexEntry(ie);

            cout << endl;
        }
    }

    void printIndexChunk()
    {
        cout << endl;
        cout << "Index entries for all streams\n";
        cout << endl;

        file.seekIndex();

        const int count = file.indexCount();

        for (int i = 0; i < count; ++i)
        {
            cout << setw(5) << setfill(' ') << i << " ";

            AVI::IndexEntry ie;

            file.read(ie);

            printIndexEntry(ie);

            cout << endl;
        }
    }

    void printAnnotatedIndexChunk()
    {
        cout << endl;

        cout << "Annotated index entries\n";

        file.seekIndex();

        const AVI::StreamHeader& sh0 = file.streamHeader(0);
        assert(sh0.fccType == "vids");

        const double rate = sh0.rate;
        const double scale = sh0.scale;

        const int count = file.indexCount();

        int frameNum = 0; //video
        int blockNum = 0; //audio

        for (int i = 0; i < count; ++i)
        {
            AVI::IndexEntry ie;

            file.read(ie);

            const int streamId = AVI::asStreamId(ie.chunkId);

            if (streamId == -1)
                continue;

            if (streamId >= file.streamCount())
                continue;

            if (streamId == 0)
            {
                cout << endl;

                const double time = static_cast<double>(frameNum) * scale / rate;

                cout << frameNum << " " << time << endl;

                ++frameNum;
            }

            printIndexEntry(ie);

            if (AVI::asChunkType(ie.chunkId) == AVI::waveform || AVI::asChunkType(ie.chunkId) == AVI::waveform_encrypted)
            {
                //const int id = AVI::asStreamId(ie.chunkId);

                const AVI::StreamHeader& sh = file.streamHeader(streamId);
                const AVI::WaveFormatEx& wfx = file.audioFormat(streamId);

                int blockCount;

                if (wfx.blockAlign) //sh.sampleSize)
                {
                    blockCount = ie.chunkLength / wfx.blockAlign;
                }
                else
                {
                    blockCount = 1;
                }

                const double r = sh.rate;
                const double s = sh.scale;

                const double currTime = double(blockNum) * s / r;

                blockNum += blockCount;

                const double nextTime = double(blockNum) * s / r;

                cout << blockCount << " " << currTime << ", " << nextTime;
            }

            cout << endl;
        }
    }

    void printOffset()
    {
        const offset_t offset = file.tell();

        const unsigned long high = static_cast<unsigned long>(offset >> 32);
        const unsigned long low = static_cast<unsigned long>(offset);

        cout << hex << setw(8) << setfill('0') << high << " " << setw(8) << setfill('0') << low << " " << dec;
    }

    void printJUNK()
    {
        printOffset();

        const FourCC code = file.readFourCC();

        assert(code == "JUNK");

        const offset_t length = file.readLength();
        const offset_t offset = file.tell() + length;

        cout << code << " (" << dec << static_cast<unsigned long>(length) << ")\n";

        file.seek(offset);
    }

    //bool isFrameChunk(const FourCC& code)
    //{
    //    const char c0 = (code & 0x00FF0000) >> 16;
    //    const char c1 = (code & 0xFF000000) >> 24;
    //
    //    if (c0 == 'd')
    //        return (c1 == 'b' || c1 == 'c');
    //
    //    else if (c0 == 'w')
    //        return (c1 == 'b');
    //
    //    else
    //        return false;
    //}

    void printChunk(int dd, int d)
    {
        printOffset();

        const FourCC code = file.readFourCC();

        const unsigned long length = file.readLength();

        offset_t offset = file.tell() + length;

        if (length % 2 == 1)
            ++offset;

        const int pad = 3 * d;

        for (int i = 0; i < pad; ++i)
            cout << " ";

        cout << code;
        cout << " (" << dec << length << ") ";

        if (code == "LIST")
        {
            const FourCC type = file.readFourCC();
            cout << "'" << type << "'\n";

            if ((dd > d) && ((type != "movi") || print_movi_chunks))
                while (file.tell() < offset)
                    printChunk(dd, d + 1);
            else
                file.seek(offset);
        }
        else
        {
            cout << endl;
            file.seek(offset);
        }
    }

    void printRIFF(int dd)
    {
        printOffset();

        const FourCC riffCode = file.readFourCC();

    //    assert(riffCode == "RIFF");

        const offset_t length = file.readLength();
        const offset_t offset = file.tell() + length;

        const FourCC aviCode = file.readFourCC();

        cout << riffCode << " (" << dec << static_cast<unsigned long>(length) << ") '" << aviCode << "'\n";

        if (dd > 0)
            while (file.tell() < offset)
                printChunk(dd, 1);
        else
            file.seek(offset);
    }

    void printKeyframes(int nStream)
    {
        const AVI::MainHeader& mh = file.mainHeader();

        if (AVI::dword(nStream) < 0 || AVI::dword(nStream) >= mh.streams)
        {
            cout << "Keyframe stream id " << nStream << " is out of range\n";
            return;
        }

        AVI::FrameIndex index;

        file.load(nStream, index);

        if (index.size() == 0)
        {
            cout << "No keyframes for stream " << nStream << endl;
            return;
        }

        cout << "Keyframes for stream " << nStream << endl;

        typedef AVI::FrameIndex::const_iterator iter_t;

        iter_t iter = index.begin();
        const iter_t iter_end = index.end();

        int fn = 0;

        while (iter != iter_end)
        {
            const AVI::FrameIndexEntry& ie = *iter;

            if (ie.keyframe)
            {
                cout << left << setw(8) << setfill(' ') << fn;
                cout << endl;
            }

            ++iter;
            ++fn;
        }
    }

    void printChunks()
    {
        const offset_t size = file.size();

        file.seek(0);

        if (depth == -1)
        {
            printRIFF(3);

            if (file.tell() < size)
            {
                printOffset();

                const FourCC fcc = file.testFourCC();

                if ((fcc == "RIFF") || (fcc == "JUNK"))
                {
                    cout << "<chunk(s) omitted>\n";
                }
                else
                {
                    const AVI::offset_t n_ = size - file.tell();
                    const unsigned long n = static_cast<unsigned long>(n_);
                    cout << '<' << dec << n << " extra bytes>\n";
                }

                file.seek(size);
            }
        }
        else
        {
            while (file.tell() < size)
            {
                const FourCC code = file.testFourCC();

                if (code == "RIFF")
                    printRIFF(depth);

                else if (code == "JUNK")
                    printJUNK();

                else
                {
                    const AVI::offset_t n_ = size - file.tell();
                    const unsigned long n = static_cast<unsigned long>(n_);
                    printOffset();
                    cout << '<' << dec << n << " extra bytes>\n";
                    file.seek(size);
                }
            }
        }

        printOffset();
        cout << "<eof>\n\n";
    }

    void streamInfo(int stream)
    {
        cout << endl;
        cout << "Stream " << stream << endl;

        cout << "index entry count: ";

        if (file.mainHeader().flags & MainHeader::hasIndex)
            cout << file.countIndexEntries(stream) << endl;
        else
            cout << "(no index)\n";

        cout << endl;

        const StreamHeader& sh = file.streamHeader(stream);
        cout << sh;
        cout << endl;

        const FourCC& fccType = sh.fccType;

        if (fccType == "vids")
        {
            cout << "Video format:\n";
            cout << file.videoFormat(stream);
        }
        else if (fccType == "auds")
        {
            cout << "Audio format:\n";

            const AVI::WaveFormatEx& af = file.audioFormat(stream);

            cout << af;
        }
        else
            assert(false);

        //  Stream Data (strd)
        cout << endl;
        cout << "strd:  ";

        const DWORD dwSize = file.streamDataSize(stream);
        const unsigned char* const pData = file.streamData(stream);

        if (dwSize == 0 || pData == 0)
        {
            cout << "<none>" << endl;
        }
        else
        {
            cout << endl;
        }

        //  Stream Name (strn)
        cout << endl;
        cout << "strn:  ";

        const char* const name = file.streamName(stream);

        if (name)
            cout << "'" << name << "'\n";
        else
            cout << "<none>" << endl;
    }

    void check_idx1()
    {
        const int j = file.streamCount();

        for (int i = 0; i < j; ++i)
        {
            cout << "checking idx1 for stream " << i << "...\n";

            AVI::IEVector iev;
            file.load(i, iev);

            typedef AVI::IEVector::const_iterator iter_t;

            iter_t iter = iev.begin();
            const iter_t iter_end = iev.end();

            AVI::offset_t base;

            if (file.indexIsRelative())
                base = file.dataOffset();
            else
                base = 0;

            int fcc_mismatch = 0;
            int len_mismatch = 0;
            int n = 0;

            while (iter != iter_end)
            {
                const AVI::IndexEntry& ie = *iter;
                const AVI::offset_t offset = base + ie.chunkOffset;

                file.seek(offset);

                const FourCC fcc = file.readFourCC();
                const unsigned long len = file.readLength();

                if (fcc != ie.chunkId)
                {
                    ++fcc_mismatch;

                    cout << "ie=" << n << " fcc mismatch: idx1=" << ie.chunkId << " frame=" << fcc << '\n';
                }

                if (len != ie.chunkLength)
                {
                    ++len_mismatch;

                    cout << "ie=" << n << " length mismatch: idx1=" << ie.chunkLength << " frame=" << len << '\n';
                }

                ++iter;
                ++n;
            }

            cout << "n=" << n << " fcc=" << fcc_mismatch << " len=" << len_mismatch << '\n';
        }

        cout << endl;
    }

    void check_indx(int i)
    {
        cout << "checking indx for stream " << i << "...\n";

        AVI::FrameIndex iev;
        file.loadIndexEx(i, iev);

        typedef AVI::FrameIndex::const_iterator iter_t;

        iter_t iter = iev.begin();
        const iter_t iter_end = iev.end();

        //int fcc_mismatch = 0;
        int len_mismatch = 0;
        int n = 0;

        while (iter != iter_end)
        {
            const AVI::FrameIndexEntry& ie = *iter;

            file.seek(ie.offset - 8);

            const FourCC fcc = file.readFourCC();
            const unsigned long len = file.readLength();

            if (len != ie.size)
            {
                ++len_mismatch;

                cout << "ie=" << n << " length mismatch: idx1=" << ie.size << " frame=" << len << '\n';
            }

            ++iter;
            ++n;
        }

        cout << "n=" << n << " len=" << len_mismatch << '\n';
    }

    void info()
    {
        cout << "\nHeader info for file '" << file.name() << "':\n" << endl;

        const AVI::MainHeader& mh = file.mainHeader();

        cout << mh << endl;

        int nExtraHeaders = file.extraHeaderCount();

        if (nExtraHeaders > 0)
        {
            cout << "Extra Header Chunks" << endl;

            for (int nChunk = 0; nChunk < nExtraHeaders; ++nChunk)
            {
                cout << "    " << file.extraHeader(nChunk) << endl;
            }
        }

        for (AVI::dword i = 0; i < mh.streams; i++)
            streamInfo(i);

        cout << endl;

        if (file.totalFrames() != -1)
            cout << "total frames (odml): " << file.totalFrames() << endl;

        const AVI::offset_t dataOffset = file.dataOffset();

        const unsigned int dataOffset_high = static_cast<unsigned int>(dataOffset >> 32);
        const unsigned int dataOffset_low = static_cast<unsigned int>(dataOffset & 0xFFFFFFFFi64);

        cout << "data offset: " << hex << setw(8) << setfill('0') << dataOffset_high << " " << setw(8) << setfill('0') << dataOffset_low << dec << endl;

        if (mh.flags & MainHeader::hasIndex)
        {
            if (file.indexIsRelative())
                cout << "index is relative\n";
            else
                cout << "index is absolute\n";

            if (printIndexSeparate)
                for (AVI::dword j = 0; j < mh.streams; ++j)
                    printIndexChunk(j);

            if (printIndexTogether)
                printIndexChunk();

            if (printAnnotated)
                printAnnotatedIndexChunk();
        }
        else
        {
            cout << "no idx1 chunk\n";
        }

        cout << endl;

        if (s_check_idx1)
        {
            if (mh.flags & MainHeader::hasIndex)
            {
                check_idx1();
            }
            else
            {
                cout << "no idx1 chunk to check\n";
            }
        }

        if (s_check_indx)
        {
            for (AVI::dword i = 0; i < mh.streams; ++i)
            {
                if (file.superIndexEntryCount(i) == -1)
                    cout << "no indx chunk for stream " << i << " to check\n";
                else
                    check_indx(i);
            }

            cout << endl;
        }

        if (printExtendedIndex != -1)
            for (AVI::dword j = 0; j < mh.streams; ++j)
            {
                if (file.superIndexEntryCount(j) == -1)
                    cout << "No extended index for stream " << j << endl;
                else
                    printExtendedIndexInfo(j);

                cout << endl;
            }

        if (fPrintKeyframes == -1)
        {
            for (AVI::dword i = 0; i < mh.streams; ++i)
            {
                printKeyframes(i);
                cout << endl;
            }
        }
        else if (fPrintKeyframes >= 0)
        {
            printKeyframes(fPrintKeyframes);
            cout << endl;
        }

        printChunks();
    }

    void parseIndexOption(const char* arg)
    {
        if (_stricmp(arg, "1") == 0)
        {
           printIndexTogether = true;
        }
        else if (_stricmp(arg, "s") == 0)
        {
           printIndexSeparate = true;
        }
        else if (*arg == 'x' || *arg == 'X')
        {
            const char* p = arg + 1;

            if (*p == '\0')
                printExtendedIndex = 0;

            else if (*p >= '0' && *p <= '9')
                printExtendedIndex = atoi(p);

            else
            {
                cerr << "bad extended index depth: " << p << endl;
                exit(1);
            }
        }
        else if (*arg == 'a' || *arg == 'A')
        {
            printAnnotated = true;
        }
        else
        {
            cerr << "bad index option arg: " << arg << endl;
            exit(1);
        }
    }

    void init(int argc, char* argv[])
    {
        if (argc == 1)
        {
            usage();
            exit(0);
        }

        const char* const options = "i:d::mk::caef:";

        for (;;)
        {
            const int c = getopt(argc, argv, options);

            if (c == -1)
                break;

            switch (c)
            {
                case 'i':
                    parseIndexOption(optarg);
                    break;

                case 'd':
                {
                    if (optarg)
                    {
                        depth = atoi(optarg);
                        assert(depth >= 0);
                    }
                    else
                    {
                        depth = 0;
                    }

                    break;
                }
                case 'm':
                    print_movi_chunks = true;
                    break;

                case 'k':
                    if (optarg)
                    {
                        fPrintKeyframes = atoi(optarg);
                        assert(fPrintKeyframes >= 0);
                    }
                    else
                    {
                        fPrintKeyframes = -1;
                    }

                    break;

                case 'f':
                {
                    if (strlen(optarg) > 1)
                    {
                        cout << "f switch value is too long\n";
                        exit(1);
                    }

                    if ((*optarg == 'x') || (*optarg == 'X'))
                    {
                        s_check_indx = true;
                    }
                    else if (*optarg == '1')
                    {
                        s_check_idx1 = true;
                    }
                    else
                    {
                        cout << "f switch value unknown\n";
                        exit(1);
                    }

                    break;
                }

                case '?':
                    cerr << "bad command line syntax\n";
                    exit(1);
                    break;

                default:
                    assert(false);
                    break;
            }
        }

        if (argc - optind == 0)
        {
            cerr << "No input file specified.\n";
            usage();
            exit(1);
        }

        const char* const name = argv[optind];

        try
        {
            file.open(name, AVI::File::in);
        }
        catch (const exception& e)
        {
            cerr << "Unable to open '" << name << "': " << e.what() << endl;
            exit(1);
        }
    }

} // namespace

int main(int argc, char* argv[])
{
    init(argc, argv);

    info();

    return 0;
}
